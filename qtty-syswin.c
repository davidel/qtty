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


#define WSA_NEEDED_VERSION MAKEWORD(2, 2)
#define BT_LOOKUP_RETRY 3
#define BT_LOOKUP_DELAY (15 * 1000)
#define QTTY_MAX_PATH 4096



typedef struct s_w32_glob_ctx {
	WIN32_FIND_DATA wfd;
	char nambuf[QTTY_MAX_PATH];
} w32_glob_ctx_t;



static char *bt_sock_cachefile(char *cfname, int len);
static int bt_sock_cachefile_lookup(FILE *file, const char *btname, BTH_ADDR *btaddr);
static int bt_sock_cache_add(FILE *file, const char *btname, BTH_ADDR const *btaddr);
static int bt_sock_cache_lookup(const char *btname, BTH_ADDR *btaddr);
static char *bt_sock_addr2str(BTH_ADDR const *btaddr, char *straddr);
static int bt_sock_str2addr(const char *straddr, BTH_ADDR *btaddr);
static int bt_sock_name2bth(const char *btname, BTH_ADDR *btaddr);
static int bt_sock_init(void);
static int bt_sock_cleanup(void);



static int wsa_refcount = 0;



static char *bt_sock_cachefile(char *cfname, int len) {
	char const *env = getenv("HOME");

	if (env)
		_snprintf(cfname, len, "%s\\.bt-namecache", env);
	else
		_snprintf(cfname, len, "c:\\.bt-namecache");

	return cfname;
}

static int bt_sock_cachefile_lookup(FILE *file, const char *btname, BTH_ADDR *btaddr) {
	char *name, *addr;
	char buf[512];

	rewind(file);
	while (fgets(buf, sizeof(buf) - 1, file) != NULL) {
		buf[strlen(buf) - 1] = 0;
		name = buf;
		if ((addr = strrchr(name, '\t')) == NULL)
			continue;
		*addr++ = '\0';
		if (bt_sock_str2addr(addr, btaddr) < 0)
			continue;
		if (_stricmp(name, btname) == 0)
			return 0;
	}

	return -1;
}

static int bt_sock_cache_add(FILE *file, const char *btname, BTH_ADDR const *btaddr) {
	BTH_ADDR laddr;
	char addr[128];

	if (bt_sock_cachefile_lookup(file, btname, &laddr) < 0) {
		fseek(file, 0, SEEK_END);
		fprintf(file, "%s\t%s\n", btname,  bt_sock_addr2str(btaddr, addr));
	}

	return 0;
}

static int bt_sock_cache_lookup(const char *btname, BTH_ADDR *btaddr) {
	int err;
	FILE *file;
	char cfname[256];

	bt_sock_cachefile(cfname, sizeof(cfname) - 1);
	if ((file = fopen(cfname, "rt")) == NULL)
		return -1;
	err = bt_sock_cachefile_lookup(file, btname, btaddr);
	fclose(file);

	return err;
}

static char *bt_sock_addr2str(BTH_ADDR const *btaddr, char *straddr) {
	int i;
	unsigned char aaddr[sizeof(BTH_ADDR)];

	memcpy(aaddr, btaddr, sizeof(BTH_ADDR));
	for (i = 0; i < 6; i++) {
		if (i)
			sprintf(straddr + 2 + (i - 1) * 3, ":%02X", aaddr[5 - i]);
		else
			sprintf(straddr, "%02X", aaddr[5]);
	}

	return straddr;
}

static int bt_sock_str2addr(const char *straddr, BTH_ADDR *btaddr) {
	int i;
	unsigned int aaddr[6];
	BTH_ADDR tmpaddr = 0;

	if (sscanf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &aaddr[0], &aaddr[1], &aaddr[2], &aaddr[3], &aaddr[4], &aaddr[5]) != 6)
		return -1;
	*btaddr = 0;
	for (i = 0; i < 6; i++) {
		tmpaddr = (BTH_ADDR) (aaddr[i] & 0xFF);
		*btaddr = ((*btaddr) << 8) + tmpaddr;
	}

	return 0;
}

static int bt_sock_name2bth(const char *btname, BTH_ADDR *btaddr) {
	int i, res, err = -1;
	HANDLE hlkp;
	ULONG flags, qssize = sizeof(WSAQUERYSET);
	PWSAQUERYSET wqs;
	FILE *file;
	char cfname[256];

	if (bt_sock_cache_lookup(btname, btaddr) == 0)
		return 0;
	bt_sock_cachefile(cfname, sizeof(cfname) - 1);
	if ((file = fopen(cfname, "r+t")) == NULL &&
	    (file = fopen(cfname, "w+t")) == NULL)
		return -1;
	if ((wqs = (PWSAQUERYSET) malloc(qssize)) == NULL) {
		fclose(file);
		return -1;
	}
	memset(wqs, 0, qssize);

	for (i = 0; i < BT_LOOKUP_RETRY; i++) {
		flags = LUP_CONTAINERS | LUP_RETURN_NAME | LUP_RETURN_ADDR;
		if (i) {
			flags |= LUP_FLUSHCACHE;
			Sleep(BT_LOOKUP_DELAY);
		}
		memset(wqs, 0, qssize);
		hlkp = NULL;
		wqs->dwNameSpace = NS_BTH;
		wqs->dwSize = sizeof(WSAQUERYSET);
		if ((res = WSALookupServiceBegin(wqs, flags, &hlkp)) != NO_ERROR ||
		    hlkp == NULL) {
			if (i) {
				free(wqs);
				fclose(file);
				return -1;
			}
			continue;
		}

		for (;;) {
			if ((res = WSALookupServiceNext(hlkp, flags,
							&qssize, wqs)) != NO_ERROR) {
				if ((res = WSAGetLastError()) == WSA_E_NO_MORE) {
					WSALookupServiceEnd(hlkp);
					free(wqs);
					fclose(file);
					return err;
				}
				if (res == WSAEFAULT) {
					free(wqs);
					if ((wqs = (PWSAQUERYSET) malloc(qssize)) == NULL) {
						WSALookupServiceEnd(hlkp);
						fclose(file);
						return -1;
					}
					memset(wqs, 0, qssize);
				}
				break;
			}
			if (wqs->lpszServiceInstanceName != NULL) {
				if (_stricmp(wqs->lpszServiceInstanceName, btname) == 0) {
					memcpy(btaddr,
					       &((PSOCKADDR_BTH) wqs->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr,
					       sizeof(*btaddr));
					err = 0;
				}
				bt_sock_cache_add(file, wqs->lpszServiceInstanceName,
						  &((PSOCKADDR_BTH) wqs->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr);
			}
		}
		WSALookupServiceEnd(hlkp);
	}
	free(wqs);
	fclose(file);

	return err;
}

static int bt_sock_init(void) {
	WSADATA wsd;

	if (wsa_refcount++ == 0) {
		if (WSAStartup(WSA_NEEDED_VERSION, &wsd)) {
			fprintf(stderr, "unable to initialize socket layer\n");
			wsa_refcount--;
			return -1;
		}
	}

	return 0;
}

static int bt_sock_cleanup(void) {

	if (--wsa_refcount == 0)
		WSACleanup();

	return 0;
}

static int bt_find_local_device(bt_sock_t sk, SOCKADDR_BTH *addr) {


	return 0;
}

bt_sock_t bt_sock_open(char const *qcaddr, int channel) {
	int alen, pisize;
	bt_sock_t sock;
	BTH_ADDR btaddr;
	SOCKADDR_BTH laddr, raddr;
	WSAPROTOCOL_INFO pinf;
	char straddr[64];

	if (bt_sock_init() < 0) {

		return INVALID_BT_SOCK;
	}
	if (bt_sock_str2addr(qcaddr, &btaddr) < 0 &&
	    bt_sock_name2bth(qcaddr, &btaddr) < 0) {
		fprintf(stderr, "unable to resolve BT name '%s'\n", qcaddr);
		bt_sock_cleanup();
		return INVALID_BT_SOCK;
	}
	memset(&raddr, 0, sizeof(raddr));
	raddr.addressFamily = AF_BTH;
	raddr.btAddr = btaddr;
	raddr.port = channel;
	if ((sock = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)) == SOCKET_ERROR) {
		perror("socket");
		bt_sock_cleanup();
		return INVALID_BT_SOCK;
	}
	pisize = sizeof(pinf);
	if (getsockopt(sock, SOL_SOCKET, SO_PROTOCOL_INFO,
		       (char *) &pinf, &pisize) != 0) {
		perror("getsockopt");
		bt_sock_cleanup();
		return INVALID_BT_SOCK;
	}
	memset(&laddr, 0, sizeof(laddr));
	laddr.addressFamily = AF_BTH;
	laddr.port = BT_PORT_ANY;
	if (bind(sock, (struct sockaddr *) &laddr, sizeof(laddr)) == -1) {
		perror("bind");
		bt_sock_close(sock);
		return INVALID_BT_SOCK;
	}
	alen = sizeof(SOCKADDR_BTH);
	if (getsockname(sock, (struct sockaddr *) &laddr, &alen) == -1) {
		perror("getsockname");
		bt_sock_close(sock);
		return INVALID_BT_SOCK;
	}
	printf("Local device %s\n", bt_sock_addr2str(&laddr.btAddr, straddr));
	printf("Remote device %s (%d)\n", qcaddr, channel);
	if(connect(sock, (struct sockaddr *) &raddr, sizeof(raddr)) == -1) {
		perror("connect");
		bt_sock_close(sock);
		return INVALID_BT_SOCK;
	}

	return sock;
}

int bt_sock_write(bt_sock_t sk, void const *data, int size) {

	return send(sk, data, size, 0);
}

int bt_sock_read(bt_sock_t sk, void *data, int size) {

	return recv(sk, data, size, 0);
}

int bt_sock_close(bt_sock_t sk) {
	int res;

	res = closesocket(sk);
	bt_sock_cleanup();

	return res;
}

int fglob_get_list(char const *path, char const *match, int recurse,
		   file_list_t **flist) {
	HANDLE hfind;
	w32_glob_ctx_t *fctx;
	file_list_t *fent;

	if ((fctx = (w32_glob_ctx_t *) malloc(sizeof(w32_glob_ctx_t))) == NULL)
		return -1;
	sprintf(fctx->nambuf, "%s\\*", path);
	if ((hfind = FindFirstFile(fctx->nambuf, &fctx->wfd)) != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(fctx->wfd.cFileName, ".") == 0 ||
			    strcmp(fctx->wfd.cFileName, "..") == 0)
				continue;
			sprintf(fctx->nambuf, "%s\\%s", path, fctx->wfd.cFileName);
			if (fctx->wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (recurse &&
				    fglob_get_list(fctx->nambuf, match, recurse, flist) < 0) {
					FindClose(hfind);
					free(fctx);
					return -1;
				}
			} else {
				if (match != NULL && !wildmatchi(fctx->wfd.cFileName, match))
					continue;
				if ((fent = (file_list_t *)
				     malloc(sizeof(file_list_t) + strlen(fctx->nambuf) + 1)) == NULL) {
					FindClose(hfind);
					free(fctx);
					return -1;
				}
				strcpy(fent->name, fctx->nambuf);
				fent->next = *flist;
				*flist = fent;
			}
		} while (FindNextFile(hfind, &fctx->wfd));
		FindClose(hfind);
	}
	free(fctx);

	return 0;
}

