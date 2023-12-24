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


#if !defined(_QTTY_SYSLIN_H)
#define _QTTY_SYSLIN_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/hci.h>
#include <readline/readline.h>
#include <readline/history.h>


#define INVALID_BT_SOCK ((bt_sock_t) -1)
#define SYS_SLASHS "/"
#define SYS_SLASHC '/'

#define SNPRINTF snprintf
#define MKDIR(p, m) mkdir(p, m)
#define PATH_EXIST(p) (access(p, 0) == 0)


typedef int bt_sock_t;
typedef uint16_t qtty_u16;
typedef uint32_t qtty_u32;
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

