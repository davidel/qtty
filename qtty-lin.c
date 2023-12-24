/*    Copyright 2023 Davide Libenzi
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

