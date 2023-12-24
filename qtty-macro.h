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

#if !defined(_QTTY_MACRO_H)
#define _QTTY_MACRO_H


#define COUNT_OF(a) (sizeof(a) / sizeof(a[0]))
#define STRSIZE(s) (sizeof(s) - 1)
#define ISCMD(p, n, s) ((n) >= (int) STRSIZE(s) && memcmp(p, s, STRSIZE(s)) == 0 && \
				((p)[STRSIZE(s)] == ' ' || (p)[STRSIZE(s)] == 0))
#define LOCHAR(c) (((c) >= 'A' && (c) <= 'Z') ? (c) + ('a' - 'A'): (c))
#define GET_LE16(v, p) do { \
	unsigned char const *__p = (unsigned char const *) p; \
	(v) = (((qtty_u16) __p[1]) << 8) | ((qtty_u16) __p[0]); \
} while (0)
#define PUT_LE16(v, p) do { \
	unsigned char *__p = (unsigned char *) p; \
	*__p++ = (unsigned char) (v); \
	*__p = (unsigned char) ((v) >> 8); \
} while (0)
#define GET_LE32(v, p) do { \
	unsigned char const *__p = (unsigned char const *) p; \
	(v) = (((unsigned long) __p[3]) << 24) | (((unsigned long) __p[2]) << 16) | \
		(((unsigned long) __p[1]) << 8) | ((unsigned long) __p[0]); \
} while (0)
#define PUT_LE32(v, p) do { \
	unsigned char *__p = (unsigned char *) p; \
	*__p++ = (unsigned char) (v); \
	*__p++ = (unsigned char) ((v) >> 8); \
	*__p++ = (unsigned char) ((v) >> 16); \
	*__p = (unsigned char) ((v) >> 24); \
} while (0)


#endif

