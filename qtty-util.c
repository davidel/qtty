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



static int iswild(char const *str);
static int really_write(bt_sock_t fd, char const *data, int size);
static int really_read(bt_sock_t fd, char *data, int size);



static int iswild(char const *str) {

	for (; *str; str++)
		if (strchr("*?", *str) != NULL)
			return 1;

	return 0;
}

static int really_write(bt_sock_t fd, char const *data, int size) {
	int count, curr;

	for (count = 0; count < size;) {
		curr = bt_sock_write(fd, data + count, size - count);
		if (curr <= 0)
			break;
		count += curr;
	}

	return size;
}

int send_pkt(bt_sock_t fd, char const *data, int size) {
	char buf[8];

	buf[0] = 0;
	PUT_LE16((unsigned int) size, buf + 1);
	if (really_write(fd, buf, 3) != 3)
		return -1;
	if (size > 0 && really_write(fd, data, size) != size)
		return -1;

	return 0;
}

static int really_read(bt_sock_t fd, char *data, int size) {
	int count, curr;

	for (count = 0; count < size;) {
		curr = bt_sock_read(fd, data + count, size - count);
		if (curr <= 0)
			break;
		count += curr;
	}

	return size;
}

int recv_pkt(bt_sock_t fd, char **data, int *size) {
	unsigned int wsize;
	char buf[8];

	if (really_read(fd, buf, 3) != 3)
		return -1;
	GET_LE16(wsize, buf + 1);
	if ((*data = (char *) malloc(wsize + 1)) == NULL)
		return -1;
	if (wsize && really_read(fd, *data, wsize) != (int) wsize) {
		free(*data);
		return -1;
	}
	(*data)[wsize] = 0;
	*size = (int) wsize;

	return 0;
}

int handle_bounce_cmd(bt_sock_t qfd, char *line, FILE *fout) {
	int size;
	char *data;

	if (send_pkt(qfd, line, strlen(line)) < 0)
		return -1;
	for (;;) {
		if (recv_pkt(qfd, &data, &size) < 0)
			return -1;
		if (!size) {
			free(data);
			break;
		}
		fwrite(data, 1, size, fout);
		free(data);
	}

	return 0;
}

int get_cmd(bt_sock_t qfd, char const *cmd,
	    int (*dproc)(void *, void const *, int), void *priv, FILE *flerr) {
	int size;
	unsigned int fsize, tsize;
	char *data;

	if (send_pkt(qfd, cmd, strlen(cmd)) < 0 ||
	    recv_pkt(qfd, &data, &size) < 0)
		return -1;
	if (size) {
		fwrite(data, 1, size, flerr);
		free(data);
		if (recv_pkt(qfd, &data, &size) < 0)
			return -1;
		free(data);
		return 1;
	}
	free(data);
	if (recv_pkt(qfd, &data, &size) < 0)
		return -1;
	if (size != 4) {
		free(data);
		return -1;
	}
	GET_LE32(fsize, data);
	free(data);
	for (tsize = 0;;) {
		if (recv_pkt(qfd, &data, &size) < 0)
			return -1;
		if (!size) {
			free(data);
			break;
		}
		if ((*dproc)(priv, data, size) != size) {
			free(data);
			return -1;
		}
		free(data);
		tsize += size;
	}
	if (tsize != fsize) {
		fprintf(flerr, "Remote read error (data size mismatch: %u/%u)\n",
			tsize, fsize);
		return 1;
	}

	return 0;
}

int put_cmd(bt_sock_t qfd, char const *cmd, unsigned int fsize,
	    int (*rproc)(void *, void *, int), void *priv, FILE *flerr) {
	int size;
	unsigned int tsize, curr;
	char *data;
	char buf[1024 * 8];

	if (send_pkt(qfd, cmd, strlen(cmd)) < 0)
		return -1;
	if (recv_pkt(qfd, &data, &size) < 0)
		return -1;
	if (size) {
		fwrite(data, 1, size, flerr);
		free(data);
		if (recv_pkt(qfd, &data, &size) < 0)
			return -1;
		free(data);
		return 1;
	}
	free(data);
	PUT_LE32(fsize, buf);
	if (send_pkt(qfd, buf, 4) < 0)
		return -1;
	for (tsize = 0; tsize < fsize;) {
		curr = sizeof(buf);
		if (curr + tsize > fsize)
			curr = fsize - tsize;
		if ((*rproc)(priv, buf, (int) curr) != (int) curr)
			return -1;
		if (send_pkt(qfd, buf, (int) curr) < 0)
			return -1;
		tsize += curr;
	}
	if (send_pkt(qfd, "", 0) < 0 ||
	    recv_pkt(qfd, &data, &size) < 0)
		return -1;
	if (size) {
		fwrite(data, 1, size, flerr);
		free(data);
		if (recv_pkt(qfd, &data, &size) < 0)
			return -1;
		free(data);
		return 1;
	}
	free(data);

	return 0;
}

int dump_to_file(void *priv, void const *data, int size) {

	return fwrite(data, 1, size, (FILE *) priv);
}

int read_from_file(void *priv, void *data, int size) {

	return fread(data, 1, size, (FILE *) priv);
}

int handle_getchunk(bt_sock_t qfd, char *line, FILE *flerr) {
	int res;
	FILE *file;
	char *dline, *remote, *local;
	char cmd[512];

	if (!(dline = strdup(line))) {
		perror("malloc");
		return 1;
	}
	if (!strtok(dline, " \t") ||
	    !(remote = strtok(NULL, " \t")) ||
	    !(local = strtok(NULL, " \t"))) {
		free(dline);
		fprintf(flerr, "Invalid command: %s\n", line);
		return 1;
	}
	if ((file = fopen(local, "wb")) == NULL) {
		perror(local);
		free(dline);
		return 1;
	}

	SNPRINTF(cmd, sizeof(cmd), "get $chk.%s", remote);
	cmd[sizeof(cmd) - 1] = 0;

	res = get_cmd(qfd, cmd, dump_to_file, (void *) file, flerr);

	fclose(file);
	if (res)
		remove(local);
	free(dline);

	return res;
}

int prepare_path(char const *path) {
	int res;
	char *dline, *slsh, *tmpp;

	if (!(dline = strdup(path))) {
		perror("malloc");
		return 1;
	}
	if ((slsh = strrchr(dline, SYS_SLASHC)) == NULL) {
		free(dline);
		return 0;
	}
	*slsh = 0;
	if (PATH_EXIST(dline) ||
	    (tmpp = strchr(dline, SYS_SLASHC)) == NULL) {
		free(dline);
		return 0;
	}
	for (;;) {
		if ((slsh = strchr(tmpp + 1, SYS_SLASHC)) == NULL)
			break;
		*slsh = 0;
		if (!PATH_EXIST(dline) && MKDIR(dline, 0775)) {
			free(dline);
			return -1;
		}
		*slsh = SYS_SLASHC;
		tmpp = slsh;
	}
	res = MKDIR(dline, 0775);
	free(dline);

	return res ? -1: 0;
}

int local_get(bt_sock_t qfd, char const *remote, char const *local, FILE *flerr) {
	int res;
	FILE *file;
	char cmd[512];

	if ((file = fopen(local, "wb")) == NULL) {
		perror(local);
		return 1;
	}

	SNPRINTF(cmd, sizeof(cmd), "get %s", remote);
	cmd[sizeof(cmd) - 1] = 0;

	res = get_cmd(qfd, cmd, dump_to_file, (void *) file, flerr);

	fclose(file);
	if (res)
		remove(local);

	return res;
}

int handle_mget(bt_sock_t qfd, char *line, FILE *flerr) {
	int res, recurse = 0, len;
	char *dline, *remote, *local, *flags, *fslh;
	char const *match = NULL;

	if (!(dline = strdup(line))) {
		perror("malloc");
		return 1;
	}
	if (!strtok(dline, " \t") ||
	    !(remote = strtok(NULL, " \t")) ||
	    !(local = strtok(NULL, " \t"))) {
		free(dline);
		fprintf(flerr, "Invalid command: %s\n", line);
		return 1;
	}
	if (*remote == '-') {
		flags = remote;
		remote = local;
		if (!(local = strtok(NULL, " \t"))) {
			free(dline);
			fprintf(flerr, "Invalid command: %s\n", line);
			return 1;
		}
		for (flags++; *flags; flags++)
			switch (*flags) {
			case 'R':
				recurse++;
				match = "*";
				break;
			}
	}
	if ((fslh = strrchr(remote, '\\')) != NULL &&
	    iswild(fslh + 1)) {
		*fslh = 0;
		match = fslh + 1;
	}
	if (local[len = strlen(local) - 1] == SYS_SLASHC)
		local[len] = 0;
	if (remote[len = strlen(remote) - 1] == '\\')
		remote[len] = 0;

	if (match)
		res = do_mget(qfd, remote, match, recurse, local, flerr);
	else
		res = local_get(qfd, remote, local, flerr);

	free(dline);

	return res;
}

int handle_mput(bt_sock_t qfd, char *line, FILE *flerr) {
	int res, recurse = 0, len;
	char *dline, *remote, *local, *flags, *fslh;
	char const *match = NULL, *pcmd;

	if (!(dline = strdup(line))) {
		perror("malloc");
		return 1;
	}
	if (!(pcmd = strtok(dline, " \t")) ||
	    !(remote = strtok(NULL, " \t")) ||
	    !(local = strtok(NULL, " \t"))) {
		free(dline);
		fprintf(flerr, "Invalid command: %s\n", line);
		return 1;
	}
	if (*remote == '-') {
		flags = remote;
		remote = local;
		if (!(local = strtok(NULL, " \t"))) {
			free(dline);
			fprintf(flerr, "Invalid command: %s\n", line);
			return 1;
		}
		for (flags++; *flags; flags++)
			switch (*flags) {
			case 'R':
				recurse++;
				match = "*";
				break;
			case 'f':
				pcmd = "putf";
				break;
			}
	}
	if ((fslh = strrchr(local, SYS_SLASHC)) != NULL &&
	    iswild(fslh + 1)) {
		*fslh = 0;
		match = fslh + 1;
	}
	if (local[len = strlen(local) - 1] == SYS_SLASHC)
		local[len] = 0;
	if (remote[len = strlen(remote) - 1] == '\\')
		remote[len] = 0;

	if (match)
		res = do_mput(qfd, remote, match, recurse, local, flerr);
	else
		res = local_put(qfd, pcmd, remote, local, flerr);

	free(dline);

	return res;
}

int local_put(bt_sock_t qfd, char const *pcmd, char const *remote, char const *local,
	      FILE *flerr) {
	int res;
	long fsize;
	FILE *file;
	char cmd[512];

	if ((file = fopen(local, "rb")) == NULL) {
		perror(local);
		return 1;
	}
	fseek(file, 0, SEEK_END);
	fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	SNPRINTF(cmd, sizeof(cmd), "%s %s", pcmd, remote);
	cmd[sizeof(cmd) - 1] = 0;

	res = put_cmd(qfd, cmd, fsize, read_from_file, (void *) file, flerr);

	fclose(file);

	return res;
}

int handle_cat(bt_sock_t qfd, char *line, FILE *flerr) {
	int res;
	char *dline, *remote;
	char cmd[512];

	if (!(dline = strdup(line))) {
		perror("malloc");
		return 1;
	}
	if (!strtok(dline, " \t") ||
	    !(remote = strtok(NULL, " \t"))) {
		free(dline);
		fprintf(flerr, "Invalid command: %s\n", line);
		return 1;
	}

	SNPRINTF(cmd, sizeof(cmd), "get %s", remote);
	cmd[sizeof(cmd) - 1] = 0;

	res = get_cmd(qfd, cmd, dump_to_file, (void *) stdout, flerr);

	free(dline);

	return res;
}

int handle_command(bt_sock_t qfd, char *line, FILE *flcons) {
	int res, len = strlen(line);

	res = 0;
	if (ISCMD(line, len, "shutdown") || ISCMD(line, len, "exit") ||
	    ISCMD(line, len, "reboot")) {
		handle_bounce_cmd(qfd, line, flcons);
		res = -1;
	} else if (ISCMD(line, len, "get")) {
		res = handle_mget(qfd, line, flcons);
	} else if (ISCMD(line, len, "put")) {
		res = handle_mput(qfd, line, flcons);
	} else if (ISCMD(line, len, "cat")) {
		res = handle_cat(qfd, line, flcons);
	} else if (ISCMD(line, len, "getchk")) {
		res = handle_getchunk(qfd, line, flcons);
	} else
		res = handle_bounce_cmd(qfd, line, flcons);

	return res;
}

char *trim_line(char *line, char const *tstr) {
	char *base, *top;

	for (base = line; *base && strchr(tstr, *base); base++);
	for (top = base; *top; top++);
	for (; top > base && strchr(tstr, top[-1]); top--);
	*top = 0;
	if (base != line)
		memmove(line, base, top - base + 1);
	return line;
}

int do_login(bt_sock_t qfd, char const *wline, char const *user, char const *passwd,
	     FILE *flerr) {
	int i, size;
	char *data;
	char const *rstr, *tmp;
	sha1_ctx_t sctx;
	unsigned char digest[20];
	char sdbuf[2 * 20 + 1];

	if ((rstr = strchr(wline, '<')) == NULL)
		return -1;
	rstr++;
	if ((tmp = strchr(rstr, '>')) == NULL)
		return -1;
	sha1_init(&sctx);
	sha1_update(&sctx, (unsigned char const *) rstr, (unsigned int) (tmp - rstr));
	sha1_update(&sctx, (unsigned char const *) ",", 1);
	sha1_update(&sctx, (unsigned char const *) passwd, strlen(passwd));
	sha1_final(digest, &sctx);
	for (i = 0; i < (int) sizeof(digest); i++)
		sprintf(sdbuf + 2 * i, "%02x", (unsigned int) digest[i]);
	if (send_pkt(qfd, user, strlen(user)) < 0 ||
	    send_pkt(qfd, sdbuf, strlen(sdbuf)) < 0 ||
	    recv_pkt(qfd, &data, &size) < 0)
		return -1;
	if (size) {
		fwrite(data, 1, size, flerr);
		free(data);
		return -1;
	}
	free(data);

	return 0;
}

void usage(char const *prg) {

	fprintf(stderr,
		"QTTY - Terminal console for Symbian QConsole server over BlueTooth network\n"
		"\tVersion %s - by Davide Libenzi <davidel@xmailserver.org>\n\n"
		"use: %s --qc-addr BADDR --qc-channel BCHAN\n"
		"\t--user USER --pass PASS [--help]\n\n", QTTY_VERSION, prg);
}

char *stristr(char const *str, char const *sstr) {
	int sslen = strlen(sstr), mpos = 0, mcc = LOCHAR(*sstr);

	if (!sslen)
		return (char *) str;
	for (; *str; str++)
		if (LOCHAR(*str) == mcc)
		{
			if (++mpos == sslen)
				return (char *) str - sslen + 1;
			mcc = LOCHAR(sstr[mpos]);
		}
	else if (mpos)
		mpos = 0, mcc = LOCHAR(*sstr);

	return NULL;
}

int wildmatch(char const *str, char const *match)
{
	int prev, mhit, revr, esc;

	for (; *match; str++, match++) {
		switch (*match) {
		case '\\':
			if (!*++match)
				return 0;
		default:
			if (*str != *match)
				return 0;
			continue;
		case '?':
			if (*str == '\0')
				return 0;
			continue;
		case '*':
			while (*(++match) == '*');
			if (!*match)
				return 1;
			while (*str)
				if (wildmatch(str++, match))
					return 1;
			return 0;
		case '[':
			esc = 0;
			revr = match[1] == '^' ? 1 : 0;
			if (revr)
				match++;
			for (prev = 256, mhit = 0; *++match &&
			     (esc || *match != ']');
			     prev = esc ? prev : *match) {
				if (!esc && (esc = *match == '\\'))
					continue;
				if (!esc && *match == '-') {
					if (!*++match)
						return 0;
					if (*match == '\\')
						if (!*++match)
							return 0;
					mhit = mhit || (*str <= *match &&
							*str >= prev);
				}
				else
					mhit = mhit || *str == *match;
				esc = 0;
			}
			if (prev == 256 || esc || *match != ']' || mhit == revr)
				return 0;
			continue;
		}
	}

	return *str == '\0' ? 1 : 0;
}

int wildmatchi(char const *str, char const *match) {
	int res, i, slen, mlen;
	char *lstr, *lmatch;

	slen = strlen(str);
	mlen = strlen(match);
	if ((lstr = (char *) malloc(slen + mlen + 2)) == NULL)
		return 0;
	lmatch = lstr + slen + 1;
	for (i = 0; i < slen; i++)
		lstr[i] = LOCHAR(str[i]);
	lstr[i] = 0;
	for (i = 0; i < mlen; i++)
		lmatch[i] = LOCHAR(match[i]);
	lmatch[i] = 0;

	res = wildmatch(lstr, lmatch);

	free(lstr);

	return res;
}

int get_file_list(bt_sock_t qfd, char const *rpath, char const *match, int recurse,
		  file_list_t **flist) {
	int size;
	char *data, *tmps;
	file_list_t *fent;
	char cmd[512];

	SNPRINTF(cmd, sizeof(cmd) - 1, "find -s%s %s %s", recurse ? "": "1",
		 rpath, match);
	if (send_pkt(qfd, cmd, strlen(cmd)) < 0)
		return -1;
	for (;;) {
		if (recv_pkt(qfd, &data, &size) < 0)
			return -1;
		if (!size) {
			free(data);
			break;
		}
		if ((fent = (file_list_t *)
		     malloc(sizeof(file_list_t) + size + 1)) == NULL) {
			free(data);
			return -1;
		}
		memcpy(fent->name, data, size);
		if ((tmps = (char *) memchr(fent->name, '\n', size)) != NULL)
			*tmps = 0;
		fent->next = *flist;
		*flist = fent;
		free(data);
	}

	return 0;
}

char *normalize_path(char *path, int sc) {
	int i;

	for (i = 0; path[i]; i++)
		switch (path[i]) {
		case '/':
		case '\\':
			path[i] = (char) sc;
			break;
		}

	return path;
}

int do_mget(bt_sock_t qfd, char const *rpath, char const *match, int recurse,
	    char const *lpath, FILE *flerr) {
	int res;
	char *pfname;
	file_list_t *flist = NULL, *fcur;
	char lfile[1024];

	if (get_file_list(qfd, rpath, match, recurse, &flist) < 0)
		return -1;
	for (fcur = flist; fcur != NULL; fcur = fcur->next) {
		if ((pfname = stristr(fcur->name, rpath)) == NULL)
			continue;
		pfname += strlen(rpath);
		SNPRINTF(lfile, sizeof(lfile) - 1, "%s%s", lpath, SYS_SLASHS);
		if (*pfname == '\\')
			pfname++;
		strcat(lfile, pfname);
		prepare_path(normalize_path(lfile, SYS_SLASHC));

		fprintf(flerr, "%s\n->\t%s\n", fcur->name, lfile);
		res = local_get(qfd, fcur->name, lfile, flerr);

		if (res < 0) {
			fglob_free_list(flist);
			return res;
		}
		if (res == 0)
			fprintf(flerr, "OK\n");

	}
	fglob_free_list(flist);

	return 0;
}

void fglob_free_list(file_list_t *flist) {
	file_list_t *fcur, *ffree;

	for (fcur = flist; (ffree = fcur) != NULL;) {
		fcur = fcur->next;
		free(ffree);
	}
}

int do_mput(bt_sock_t qfd, char const *rpath, char const *match, int recurse,
	    char const *lpath, FILE *flerr) {
	int res, lplen;
	char *pfname;
	file_list_t *flist = NULL, *fcur;
	char rfile[512];

	if (fglob_get_list(lpath, match, recurse, &flist) < 0) {
		fprintf(flerr, "Invalid path: %s\n", lpath);
		return 1;
	}
	for (fcur = flist, lplen = strlen(lpath); fcur != NULL;
	     fcur = fcur->next) {
		pfname = fcur->name + lplen;
		if (*pfname == SYS_SLASHC)
			pfname++;
		SNPRINTF(rfile, sizeof(rfile) - 1, "%s\\%s", rpath, pfname);
		normalize_path(rfile, '\\');

		fprintf(flerr, "%s\n->\t%s\n", fcur->name, rfile);
		res = local_put(qfd, "putf", rfile, fcur->name, flerr);

		if (res < 0) {
			fglob_free_list(flist);
			return res;
		}
		if (res == 0)
			fprintf(flerr, "OK\n");
	}
	fglob_free_list(flist);

	return 0;
}

