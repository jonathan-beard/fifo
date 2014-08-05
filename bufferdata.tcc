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
   Element()
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

struct Signal
{
   Signal() : sig( RBSignal::NONE )
   {
   }

   Signal( const Signal &other )
   {
      (this)->sig = other.sig;
   }

   RBSignal sig;
};

template < class T, 
           RingBufferType B = RingBufferType::Heap, 
           size_t SIZE = 0 > struct Data
{


   Data( size_t max_cap , const size_t align = 16 ) : read_pt( max_cap ),
                                                      write_pt( max_cap ),
                                                      max_cap( max_cap ),
                                                      store( nullptr ),
                                                      signal( nullptr )
   {
      const auto length_store( ( sizeof( Element< T > ) * max_cap ) ); 
      const auto length_signal( ( sizeof( Signal ) * max_cap ) );
      int ret_val( posix_memalign( (void**)&((this)->store), 
                                   align, 
                                   length_store ) );
      if( ret_val != 0 )
      {
         std::cerr << "posix_memalign returned error code (" << ret_val << ")";
         std::cerr << " with message: \n" << strerror( ret_val ) << "\n";
         exit( EXIT_FAILURE );
      }
      
      errno = 0;
      (this)->signal = (Signal*)       calloc( length_signal,
                                               sizeof( Signal ) );
      if( (this)->signal == nullptr )
      {
         perror( "Failed to allocate signal queue!" );
         exit( EXIT_FAILURE );
      }
   }


   ~Data()
   {
      std::memset( (this)->store, 0, ( sizeof( Element< T > ) * max_cap ) );
      free( (this)->store );
      std::memset( (this)->signal, 0, ( sizeof( Signal ) * max_cap ) );
      free( (this)->signal );
   }

   Pointer           read_pt;
   Pointer           write_pt;
   size_t            max_cap;
   /** 
    * allocating these as structs gives a bit
    * more flexibility later in what to pass
    * along with the queue.  It'll be more 
    * efficient copy wise to pass extra items
    * in the signal, but conceivably there could
    * be a case for adding items in the store
    * as well.
    */
   Element< T >      *store;
   Signal            *signal;
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
