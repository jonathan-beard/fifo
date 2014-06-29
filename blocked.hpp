/**
 * blocked.hpp - 
 * @author: Jonathan Beard
 * @version: Sun Jun 29 14:06:10 2014
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
#ifndef _BLOCKED_HPP_
#define _BLOCKED_HPP_  1

typedef std::uint32_t blocked_part_t;
typedef std::uint64_t blocked_whole_t;

union Blocked{
   
   Blocked() : all( 0 )
   {}

   Blocked( volatile Blocked &other )
   {
      all = other.all;
   }

   struct{
      blocked_part_t  count;
      blocked_part_t  blocked;
   };
   blocked_whole_t    all;
} __attribute__ ((aligned( 8 )));
#endif /* END _BLOCKED_HPP_ */
