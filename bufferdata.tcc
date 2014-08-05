/**
 * bufferdata.tcc - 
 * @author: Jonathan Beard
 * @version: Fri May 16 13:08:25 2014
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
#ifndef _BUFFERDATA_TCC_
#define _BUFFERDATA_TCC_  1
#include <cstdint>
#include "pointer.hpp"
#include "ringbuffertypes.hpp"
#include <cstring>
#include <cassert>
#include "signalvars.hpp"

namespace Buffer
{

/**
 * Element - simple struct that carries with it 
 * a ``signal'' to enable synchronous signaling
 * that is element aligned.  The send_signal()
 * function enables asynchronous signaling with
 * the same signal type (RBSignal).
 */
template < class X > struct Element
{
   /** default constructor **/
   Element() : signal( RBSignal::NONE )
   {
   }
   /**
    * Element - copy constructor, the type
    * X must have a defined assignment operator.
    * This is simple for primitive types, but
    * probably must be defined for objects,
    * structs and more complex types.
    */
   Element( const Element< X > &other )
   {
      (this)->item   = other.item;
   }

   X item;
};



template < class T, 
           RingBufferType B = RingBufferType::Heap, 
           size_t SIZE = 0 > struct Data
{


   Data( size_t max_cap ) : read_pt( max_cap ),
                            write_pt( max_cap ),
                            max_cap( max_cap ),
                            store( nullptr ),
                            signal( nullptr ),
                            ptr( nullptr );
   {
      errno = 0;
      const auto length_store( ( sizeof( Element< T > ) * max_cap ) ); 
      const auto length_signal( ( sizeof( RBSignal ) * max_cap ) );
      const auto ptr_size( length_store + length_signal );
      (this)->ptr = (char*) calloc( ptr_size );
      assert( (this)->ptr != nullptr );
      (this)->store  = reinterpret_cast< Element< T >* >( ptr );
      (this)->signal = reinterpret_cast< RBSignal* >( &( ptr[ length_store ] ));
   }


   ~Data()
   {
      std::memset( store, 0, sizeof( Element< T > ) * max_cap );
      delete( store );
      store = nullptr;
   }

   Pointer           read_pt;
   Pointer           write_pt;
   size_t            max_cap;
   Element< T >      *store;
   RBSignal          *signal;
   /**
    * ptr is the location and main ptr to free
    * when deallocating the store and signal ptrs.
    */
   char              *ptr;
};

template < class T, 
           size_t SIZE > struct Data< T, 
                                      RingBufferType::SharedMemory, 
                                      SIZE >
{
   Data( size_t max_cap ) : read_pt( max_cap ),
                            write_pt( max_cap ),
                            max_cap( max_cap )
   {

   }

   Pointer read_pt;
   Pointer write_pt;
   size_t  max_cap;
   struct  Cookie
   {
      Cookie() : a( 0 ),
                 b( 0 )
      {
      }
      volatile uint32_t a;
      volatile uint32_t b;
   }       cookie;
   Element< T >       store[  SIZE ];
   RBSignal           signal[ SIZE ];
};
}
#endif /* END _BUFFERDATA_TCC_ */
