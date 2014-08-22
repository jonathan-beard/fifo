/**
 * sampletype.tcc - 
 * @author: Jonathan Beard
 * @version: Thu Aug 21 12:02:32 2014
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
#ifndef _SAMPLETYPE_TCC_
#define _SAMPLETYPE_TCC_  1

#include <vector>
#include <string>
#include <ostream>
#include "ringbuffertypes.hpp"
#include "ringbufferbase.tcc"

template < class T, RingBufferType type > class SampleType
{
public:
virtual void sample( RingBufferBase< T, type >     &buffer ) = 0;
virtual void accept( volatile bool &converged ) = 0;

std::ostream&
print( std::ostream &stream )
{
   stream << (this)->printHeader() << ", " << (this)->printData();
   return( stream );
}

protected:
virtual std::string printHeader() = 0;
virtual std::string printData()   = 0;


};
#endif /* END _SAMPLETYPE_TCC_ */
