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

enum class ErrorSignals : std::int64_t 
{
   NONE = 0,
   SIGHUP,
   SIGQUIT,
   SIGILL,
   SIGTRAP,
   SIGABRT,
   SIGFPE,
   SIGKILL,
   SIGBUG,
   SIGSEGV,
   SIGSYS,
   SIGPIPE,
   SIGALRM,
   SIGTERM,
   SIGURG,
   SIGSTOP,
   SIGTSTP,
   SIGCONT,
   SIGCHLD,
   SIGTTIN,
   SIGTTOU,
   SIGIO,
   SIGXCPU,
   SIGXFSZ,

}


#endif /* END _SIGNALVARS_HPP_ */
