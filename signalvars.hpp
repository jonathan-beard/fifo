/**
 * signalvars.hpp - 
 * @author: Jonathan Beard
 * @version: Sun May 11 08:26:13 2014
 * 
 * Copyright 2014 Jonathan Beard
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _SIGNALVARS_HPP_
#define _SIGNALVARS_HPP_  1
#ifdef __cplusplus
extern "C"{
#endif
#include <stdint.h>
typedef struct Signal
{
   union{
   uint64_t 
   SIGHUP   :1,
   SIGQUIT   :1,
   SIGILL   :1,
   SIGTRAP   :1,
   SIGABRT   :1,
   SIGFPE   :1,
   SIGKILL   :1,
   SIGBUG   :1,
   SIGSEGV   :1,
   SIGSYS   :1,
   SIGPIPE   :1,
   SIGALRM   :1,
   SIGTERM   :1,
   SIGURG   :1,
   SIGSTOP   :1,
   SIGTSTP   :1,
   SIGCONT   :1,
   SIGCHLD   :1,
   SIGTTIN   :1,
   SIGTTOU   :1,
   SIGIO   :1,
   SIGXCPU   :1,
   SIGXFSZ   :1,
   SIGEOD    :1,
             :39;
   uint64_t all;
   };
};

#ifdef __cplusplus
}
#endif

#endif /* END _SIGNALVARS_HPP_ */
