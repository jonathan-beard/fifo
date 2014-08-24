/**
 * ratesampletype.tcc - 
 * @author: Jonathan Beard
 * @version: Sat Aug 23 16:56:48 2014
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
#ifndef _RATESAMPLETYPE_TCC_
#define _RATESAMPLETYPE_TCC_  1
#include <string>
#include "ringbufferbase.tcc"
#include "ringbuffertypes.hpp"

template < class T, RingBufferType type > class RateSampleType : 
   public SampleType< T, type >
{
public:
RateSampleType() : SampleType< T, type >()
{
}

virtual ~RateSampleType()
{

}

protected:
struct stats
{
   stats() : items_copied( 0 ),
             frame_count( 0 )
   {
   }

   stats&
   operator += (const stats &rhs )
   {
      (this)->items_copied += rhs.item_copied;
      (this)->frame_count  += 1;
      return( *this );
   }

   std::int64_t items_copied;
   std::int64_t frame_count;
} real, temp;
};
#endif /* END _RATESAMPLETYPE_TCC_ */
