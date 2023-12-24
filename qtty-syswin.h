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

#if !defined(_QTTY_SYSWIN_H)
#define _QTTY_SYSWIN_H

#include <winsock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <direct.h>


#define INVALID_BT_SOCK INVALID_SOCKET
#define SYS_SLASHS "\\"
#define SYS_SLASHC '\\'

#define SNPRINTF _snprintf
#define MKDIR(p, m) _mkdir(p)
#define PATH_EXIST(p) (_access(p, 0) == 0)


typedef SOCKET bt_sock_t;
typedef unsigned short qtty_u16;
typedef unsigned int qtty_u32;
typedef struct s_file_list {
	struct s_file_list *next;
	char name[1];
} file_list_t;


bt_sock_t bt_sock_open(char const *qcaddr, int channel);
int bt_sock_write(bt_sock_t sk, void const *data, int size);
int bt_sock_read(bt_sock_t sk, void *data, int size);
int bt_sock_close(bt_sock_t sk);
int fglob_get_list(char const *path, char const *match, int recurse,
		   file_list_t **flist);


#endif

