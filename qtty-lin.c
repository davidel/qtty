/*
 *  QTTY by Davide Libenzi (Terminal interface to QConsole/WmConsole)
 *  Copyright (C) 2004  Davide Libenzi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#include "qtty.h"


#define QTTY_HISTORY_FILE ".qtty_history"


static bt_sock_t qcfd;
static volatile int qquit, sigexit;



static int rdln_event_hook(void) {
	struct pollfd pfd;

	pfd.fd = qcfd;
	pfd.events = POLLIN | POLLOUT;
	pfd.revents = 0;
	poll(&pfd, 1, 0);

	if (pfd.revents & (POLLERR | POLLHUP)) {
		fprintf(stderr, "\nRemote server shut down\n");
		rl_done = 1;
		qquit++;
	}

	return 0;
}

static void break_handler(int sig) {

	rl_done = 1;
	qquit++;
	sigexit++;
}

int main(int ac, char **av) {
	int i, size, channel = -1;
	char *prompt = "$ ", *line, *user = NULL, *passwd = NULL, *qcaddr = NULL;
	char const *home;
	char hfile[256];

	signal(SIGQUIT, break_handler);
	signal(SIGINT, break_handler);
	rl_initialize();
	rl_event_hook = rdln_event_hook;
	if ((home = getenv("HOME")) != NULL)
		SNPRINTF(hfile, sizeof(hfile), "%s/%s", home, QTTY_HISTORY_FILE);
	else
		SNPRINTF(hfile, sizeof(hfile), "%s", QTTY_HISTORY_FILE);
	hfile[sizeof(hfile) - 1] = 0;
	read_history_range(hfile, 0, -1);
	for (i = 1; i < ac; i++) {
		if (!strcmp(av[i], "--user")) {
			if (++i < ac)
				user = av[i];
		} else if (!strcmp(av[i], "--pass")) {
			if (++i < ac)
				passwd = av[i];
		} else if (!strcmp(av[i], "--qc-addr")) {
			if (++i < ac)
				qcaddr = av[i];
		} else if (!strcmp(av[i], "--qc-channel")) {
			if (++i < ac)
				channel = atoi(av[i]);
		} else {
			usage(av[0]);
			return 1;
		}
	}
	if (!qcaddr || channel < 0 || !user || !passwd) {
		usage(av[0]);
		return 1;
	}

	fprintf(stderr, "Opening connection to %s (%d)\n", qcaddr, channel);
	fflush(stderr);
	if ((qcfd = bt_sock_open(qcaddr, channel)) == INVALID_BT_SOCK)
		return 1;
	if (recv_pkt(qcfd, &line, &size) < 0) {
		bt_sock_close(qcfd);
		return 1;
	}
	fprintf(stderr, "%s", line);
	if (do_login(qcfd, line, user, passwd, stderr) < 0) {
		free(line);
		bt_sock_close(qcfd);
		return 2;
	}
	free(line);

	for (; !qquit;) {
		line = readline(prompt);
		if (!line)
			break;
		trim_line(line, " \r\n\t");
		if (!*line) {
			free(line);
			continue;
		}
		add_history(line);

		if (handle_command(qcfd, line, stderr) < 0) {
			free(line);
			break;
		}

		free(line);
	}

	if (sigexit)
		handle_bounce_cmd(qcfd, "exit", stderr);
	bt_sock_close(qcfd);
	write_history(hfile);

	return 0;
}

