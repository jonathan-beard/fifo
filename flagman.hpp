/**
 * flagman.hpp - 
 * @author: Jonathan Beard
 * @version: Tue Jun 24 11:16:53 2014
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
#ifndef _FLAGMAN_HPP_
#define _FLAGMAN_HPP_  1
class FlagMan
{
public:
   FlagMan();
   virtual ~FlagMan();

private:
   volatile RBSignal a;
   volatile RBSignal b;
};
#endif /* END _FLAGMAN_HPP_ */
