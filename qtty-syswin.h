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

