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
#include <cstring>
#include <cassert>
#include "shm.hpp"
#include "signalvars.hpp"
#include "pointer.hpp"
#include "ringbuffertypes.hpp"

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

/**
 * DataBase - not quite the best name since we 
 * conjure up a relational database, but it is
 * literally the base for the Data structs below.
 */
template < class T > struct DataBase 
{
   DataBase( size_t max_cap ) : read_pt ( nullptr ),
                                write_pt( nullptr ),
                                max_cap ( max_cap ),
                                store   ( nullptr ),
                                signal  ( nullptr )
   {

      length_store   = ( sizeof( Element< T > ) * max_cap ); 
      length_signal  = ( sizeof( Signal ) * max_cap );
   }

   Pointer           *read_pt;
   Pointer           *write_pt;
   size_t             max_cap;
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
   size_t             length_store;
   size_t             length_signal;
};

template < class T, 
           RingBufferType B = RingBufferType::Heap, 
           size_t SIZE = 0 > struct Data : public DataBase< T >
{


   Data( size_t max_cap , const size_t align = 16 ) : DataBase< T >( max_cap )
   {
      int ret_val( posix_memalign( (void**)&((this)->store), 
                                   align, 
                                   (this)->length_store ) );
      if( ret_val != 0 )
      {
         std::cerr << "posix_memalign returned error code (" << ret_val << ")";
         std::cerr << " with message: \n" << strerror( ret_val ) << "\n";
         exit( EXIT_FAILURE );
      }
      
      errno = 0;
      (this)->signal = (Signal*)       calloc( (this)->length_signal,
                                               sizeof( Signal ) );
      if( (this)->signal == nullptr )
      {
         perror( "Failed to allocate signal queue!" );
         exit( EXIT_FAILURE );
      }
      /** allocate read and write pointers **/
      /** TODO, see if there are optimizations to be made with sizing and alignment **/
      (this)->read_pt   = new Pointer( max_cap );
      (this)->write_pt  = new Pointer( max_cap ); 
   }


   ~Data()
   {
      //DELETE USED HERE
      delete( (this)->read_pt );
      delete( (this)->write_pt );

      //FREE USED HERE
      std::memset( (this)->store, 0, ( sizeof( Element< T > ) * (this)->max_cap ) );
      free( (this)->store );
      std::memset( (this)->signal, 0, ( sizeof( Signal ) * (this)->max_cap ) );
      free( (this)->signal );
   }

};

template < class T > struct Data< T, RingBufferType::SharedMemory > : public DataBase< T > 
{
   /**
    * Data - Constructor for SHM based ringbuffer.  Allocates store, signal and 
    * ptr structures in separate SHM segments.  Could have been a single one but
    * makes reading the ptr arithmatic a bit more difficult.  TODO, reevaluate
    * if performance wise that might be a good idea, also decide how best to 
    * align data elements.
    * @param   max_cap, size_t with number of items to allocate queue for
    * @param   shm_key, const std::string key for opening the SHM, must be same for both ends of the queue
    * @param   dir,     Direction enum for letting this queue know which side we're allocating
    * @param   alignment, size_t with alignment NOTE: haven't implemented yet
    */
   Data( size_t max_cap, 
         const std::string shm_key,
         Direction dir,
         const size_t alignment ) : DataBase< T >( max_cap ),
                                    store_key( shm_key + "_store" ),
                                    signal_key( shm_key + "_key" ),
                                    ptr_key( shm_key + "_ptr" )
   {
      /** now work through opening SHM **/
      try
      {
         (this)->store  = (Element< T >*) SHM::Init( store_key.c_str(), (this)->length_store );
      }
      catch( bad_shm_alloc &ex )
      {
         try
         {
            (this)->store = (Element< T >*) SHM::Open( store_key.c_str() );
         }
         catch( bad_shm_alloc &ex2 )
         {
            std::cerr << "Error allocating SHM for store.\n";
            std::cerr << ex2.what() << "\n";
            exit( EXIT_FAILURE );
         }
      }
      assert( (this)->store != nullptr );
      /** allocate memory for signals **/
      try
      {
         (this)->signal = (Signal*) SHM::Init( signal_key.c_str(), (this)->length_signal );
      }
      catch( bad_shm_alloc &ex )
      {
         try
         {
            (this)->signal = (Signal*) SHM::Open( signal_key.c_str() );
         }
         catch( bad_shm_alloc &ex2 )
         {
            std::cerr << "Error allocating SHM for signal queue.\n";
            std::cerr << ex2.what() << "\n";
            exit( EXIT_FAILURE );
         }
      }
      assert( (this)->signal != nullptr );
      
      /** allocate memory for pointers **/
      try
      {
         (this)->read_ptr = 
            (Pointer*) SHM::Init( ptr_key.c_str(), (sizeof( Pointer ) * 2) + 
                                                    sizeof( Cookie ));
      }
      catch( bad_shm_alloc &ex )
      {
         try
         {
            (this)->read_ptr = (Pointer*) SHM::Open( ptr_key.c_str() );
            (this)->write_ptr = (this)->read_ptr[ 1 ];
            /** 
             * this will happen at least once for the SHM sections, 
             * might as well do it here
             */
            Pointer temp( max_cap );
            std::memcpy( &(this)->data->read_pt,  &temp, sizeof( Pointer ) );
            std::memcpy( &(this)->data->write_pt, &temp, sizeof( Pointer ) );
         }
         catch( bad_shm_alloc &ex2 )
         {
            std::cerr << "Error allocating SHM for read/write ptrs.\n";
            std::cerr << ex2.what() << "\n";
            exit( EXIT_FAILURE );
         }
      }
      assert( (this)->read_ptr   != nullptr );
      assert( (this)->write_ptr  != nullptr );
      /** set the cookie **/
      (this)->cookie = ((Cookie*) &(this)->read_ptr[ 2 ] );
      assert( (this)->cookie     != nullptr );
      switch( dir )
      {
         case( Direction::Producer ):
            (this)->cookie->a = 0x1337;
            break;
         case( Direction::Consumer ):
            (this)->cookie->b = 0x1337;
            break;
         default:
            std::cerr << "Unknown direction (" << dir << "), exiting!!\n";
            exit( EXIT_FAILURE );
            break;
      }
      while( (this)->data->cookie.a != (this)->data->cookie.b )
      {
         std::this_thread::yield();
      }
      /** should be all set now **/
   }

   ~Data()
   {
      /** three segments of SHM to close **/
      SHM::Close( store_key, (void*) (this)->store, (this)->length_store, true, true ); 
      SHM::Close( signal_key,(void*) (this)->signal,(this)->length_signal, true, true );
      SHM::Close( ptr_key,   (void*) (this)->read_ptr, (sizeof( Pointer ) * 2) + sizeof( Cookie ), true, true );
   }

   struct  Cookie
   {
      Cookie() : a( 0 ),
                 b( 0 )
      {
      }
      volatile uint32_t a;
      volatile uint32_t b;
   };

   Cookie *cookie;

   /** process local key copies **/
   const std::string store_key; 
   const std::string signal_key;  
   const std::string ptr_key; 
};
}
#endif /* END _BUFFERDATA_TCC_ */
