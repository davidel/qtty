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

