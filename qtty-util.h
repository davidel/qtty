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

#if !defined(_QTTY_UTIL_H)
#define _QTTY_UTIL_H



int send_pkt(bt_sock_t fd, char const *data, int size);
int recv_pkt(bt_sock_t fd, char **data, int *size);
int handle_bounce_cmd(bt_sock_t qfd, char *line, FILE *fout);
int get_cmd(bt_sock_t qfd, char const *cmd,
	    int (*dproc)(void *, void const *, int), void *priv, FILE *flerr);
int put_cmd(bt_sock_t qfd, char const *cmd, unsigned int fsize,
	    int (*rproc)(void *, void *, int), void *priv, FILE *flerr);
int dump_to_file(void *priv, void const *data, int size);
int read_from_file(void *priv, void *data, int size);
int handle_getchunk(bt_sock_t qfd, char *line, FILE *flerr);
int prepare_path(char const *path);
int local_get(bt_sock_t qfd, char const *remote, char const *local, FILE *flerr);
int handle_mget(bt_sock_t qfd, char *line, FILE *flerr);
int handle_mput(bt_sock_t qfd, char *line, FILE *flerr);
int local_put(bt_sock_t qfd, char const *pcmd, char const *remote, char const *local,
	      FILE *flerr);
int handle_cat(bt_sock_t qfd, char *line, FILE *flerr);
int handle_command(bt_sock_t qfd, char *line, FILE *flcons);
char *trim_line(char *line, char const *tstr);
int do_login(bt_sock_t qfd, char const *wline, char const *user, char const *passwd,
	     FILE *flerr);
void usage(char const *prg);
char *stristr(char const *str, char const *sstr);
int wildmatch(char const *str, char const *match);
int wildmatchi(char const *str, char const *match);
int get_file_list(bt_sock_t qfd, char const *rpath, char const *match, int recurse,
		  file_list_t **flist);
char *normalize_path(char *path, int sc);
int do_mget(bt_sock_t qfd, char const *rpath, char const *match, int recurse,
	    char const *lpath, FILE *flerr);
void fglob_free_list(file_list_t *flist);
int do_mput(bt_sock_t qfd, char const *rpath, char const *match, int recurse,
	    char const *lpath, FILE *flerr);


#endif

