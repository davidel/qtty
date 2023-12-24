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


#if !defined(_QTTY_H)
#define _QTTY_H

#define QTTY_VERSION "1.60"

#if defined(WIN32)
#include "qtty-syswin.h"
#else
#include "qtty-syslin.h"
#endif

#include "qtty-macro.h"
#include "qtty-sha1.h"
#include "qtty-util.h"



#endif

