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


#define QTTY_MAX_PATH 4096



static char *bt_sock_cachefile(char *cfname, int len);
static int bt_sock_cachefile_lookup(FILE *file, const char *btname, bdaddr_t *btaddr);
static int bt_sock_cache_add(FILE *file, const char *btname, bdaddr_t const *btaddr);
static int bt_sock_cache_lookup(const char *btname, bdaddr_t *btaddr);
static char *bt_sock_addr2str(bdaddr_t const *btaddr, char *straddr);
static int bt_sock_str2addr(const char *straddr, bdaddr_t *btaddr);
static int bt_sock_name2bth(const char *btname, bdaddr_t *btaddr);



static char *bt_sock_cachefile(char *cfname, int len) {
	char const *env = getenv("HOME");

	if (env)
		snprintf(cfname, len, "%s/.bt-namecache", env);
	else
		snprintf(cfname, len, ".bt-namecache");

	return cfname;
}

static int bt_sock_cachefile_lookup(FILE *file, const char *btname, bdaddr_t *btaddr) {
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
		if (strcasecmp(name, btname) == 0)
			return 0;
	}

	return -1;
}

static int bt_sock_cache_add(FILE *file, const char *btname, bdaddr_t const *btaddr) {
	bdaddr_t laddr;
	char addr[128];

	if (bt_sock_cachefile_lookup(file, btname, &laddr) < 0) {
		fseek(file, 0, SEEK_END);
		fprintf(file, "%s\t%s\n", btname,  bt_sock_addr2str(btaddr, addr));
	}

	return 0;
}

static int bt_sock_cache_lookup(const char *btname, bdaddr_t *btaddr) {
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

static char *bt_sock_addr2str(bdaddr_t const *btaddr, char *straddr) {
	int i;

	for (i = 0; i < 6; i++) {
		if (i)
			sprintf(straddr + 2 + (i - 1) * 3, ":%02X", btaddr->b[5 - i]);
		else
			sprintf(straddr, "%02X", btaddr->b[5]);
	}

	return straddr;
}

static int bt_sock_str2addr(const char *straddr, bdaddr_t *btaddr) {
	int i;
	unsigned int aaddr[6];

	if (sscanf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &aaddr[0], &aaddr[1], &aaddr[2], &aaddr[3], &aaddr[4], &aaddr[5]) != 6)
		return -1;
	for (i = 0; i < 6; i++)
		btaddr->b[5 - i] = (unsigned char) (aaddr[i] & 0xff);

	return 0;
}

static int bt_sock_name2bth(const char *btname, bdaddr_t *btaddr) {
	int i, niinf, dd, err = -1;
	inquiry_info *piinf = NULL;
	FILE *file;
	char cfname[256];

	if (bt_sock_cache_lookup(btname, btaddr) == 0)
		return 0;
	bt_sock_cachefile(cfname, sizeof(cfname) - 1);
	if ((file = fopen(cfname, "r+t")) == NULL &&
	    (file = fopen(cfname, "w+t")) == NULL)
		return -1;
	fprintf(stderr, "Resolving name '%s' ...\n", btname);
	if ((niinf = hci_inquiry(0, 16, -1, NULL, &piinf, 0)) < 0) {
		fprintf(stderr, "hci_inquiry error\n");
		fclose(file);
		return -1;
	}
	if ((dd = hci_open_dev(0)) < 0) {
		fprintf(stderr, "Unable to open HCI device\n");
		free(piinf);
		fclose(file);
		return -1;
	}
	for (i = 0; i < niinf; i++) {
		char devname[128];

		if (hci_remote_name(dd, &piinf[i].bdaddr, sizeof(devname) - 1,
				    devname, 100000) >= 0) {
			if (strcasecmp(devname, btname) == 0) {
				*btaddr = piinf[i].bdaddr;
				err = 0;
			}
			bt_sock_cache_add(file, devname, &piinf[i].bdaddr);
		}
	}
	hci_close_dev(dd);
	free(piinf);
	fclose(file);

	return err;
}

bt_sock_t bt_sock_open(char const *qcaddr, int channel) {
	int sock, d;
	bdaddr_t btaddr;
	struct sockaddr_rc laddr, raddr;
	struct hci_dev_info di;
	char straddr[64];

	if(hci_devinfo(0, &di) < 0) {
		perror("hci_devinfo");
		return INVALID_BT_SOCK;
	}

	if (bt_sock_str2addr(qcaddr, &btaddr) < 0 &&
	    bt_sock_name2bth(qcaddr, &btaddr) < 0) {
		fprintf(stderr, "Unable to resolve '%s'\n", qcaddr);
		return INVALID_BT_SOCK;
	}

	laddr.rc_family = AF_BLUETOOTH;
	laddr.rc_bdaddr = di.bdaddr;
	laddr.rc_channel = 0;

	raddr.rc_family = AF_BLUETOOTH;
	raddr.rc_bdaddr = btaddr;
	raddr.rc_channel = channel;

	if ((sock = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) < 0) {
		perror("socket");
		return INVALID_BT_SOCK;
	}
	if (bind(sock, (struct sockaddr *) &laddr, sizeof(laddr)) < 0) {
		perror("bind");
		close(sock);
		return INVALID_BT_SOCK;
	}
	printf("Local device %s\n", bt_sock_addr2str(&di.bdaddr, straddr));
	printf("Remote device %s (%d)\n", qcaddr, channel);

	if(connect(sock, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
		perror("connect");
		close(sock);
		return INVALID_BT_SOCK;
	}

	return sock;
}

int bt_sock_write(bt_sock_t sk, void const *data, int size) {

	return write(sk, data, size);
}

int bt_sock_read(bt_sock_t sk, void *data, int size) {

	return read(sk, data, size);
}

int bt_sock_close(bt_sock_t sk) {

	return close(sk);
}

int fglob_get_list(char const *path, char const *match, int recurse,
		   file_list_t **flist) {
	DIR *dir;
	struct dirent *dent;
	char *nambuf;
	file_list_t *fent;
	struct stat stbuf;

	if ((dir = opendir(path)) == NULL)
		return -1;
	if ((nambuf = (char *) malloc(QTTY_MAX_PATH)) == NULL) {
		closedir(dir);
		return -1;
	}
	while ((dent = readdir(dir)) != NULL) {
		if (strcmp(dent->d_name, ".") == 0 ||
		    strcmp(dent->d_name, "..") == 0)
			continue;
		snprintf(nambuf, QTTY_MAX_PATH, "%s/%s", path, dent->d_name);
		if (stat(nambuf, &stbuf))
			continue;
		if (S_ISDIR(stbuf.st_mode)) {
			if (recurse &&
			    fglob_get_list(nambuf, match, recurse, flist) < 0) {
				free(nambuf);
				closedir(dir);
				return -1;
			}
		} else if (S_ISREG(stbuf.st_mode)) {
			if (match != NULL && !wildmatch(dent->d_name, match))
				continue;
			if ((fent = (file_list_t *)
			     malloc(sizeof(file_list_t) + strlen(nambuf) + 1)) == NULL) {
				free(nambuf);
				closedir(dir);
				return -1;
			}
			strcpy(fent->name, nambuf);
			fent->next = *flist;
			*flist = fent;
		}
	}
	free(nambuf);
	closedir(dir);

	return 0;
}

