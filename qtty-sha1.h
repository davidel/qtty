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

