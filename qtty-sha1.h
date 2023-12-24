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

#if !defined(_QTTY_SHA1_H)
#define _QTTY_SHA1_H

#ifdef __cplusplus
#define SHA1_EXTC extern "C"
#else
#define SHA1_EXTC
#endif


#if !defined(SHA1_INT32TYPE)
#define SHA1_INT32TYPE int
#endif

#define SHA1_DIGEST_SIZE 20


typedef unsigned SHA1_INT32TYPE sha1_int32;

typedef struct {
	sha1_int32 state[5];
	sha1_int32 count[2];
	unsigned char buffer[64];
} sha1_ctx_t;


SHA1_EXTC void sha1_init(sha1_ctx_t *context);
SHA1_EXTC void sha1_update(sha1_ctx_t *context, unsigned char const *data,
			   unsigned int len);
SHA1_EXTC void sha1_final(unsigned char digest[SHA1_DIGEST_SIZE], sha1_ctx_t *context);

#endif

