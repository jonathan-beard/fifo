/**
 * ringbuffer.tcc - 
 * @author: Jonathan Beard
 * @version: Wed Apr 16 14:18:43 2014
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
#ifndef _RINGBUFFER_TCC_
#define _RINGBUFFER_TCC_  1

#include <array>
#include <cstdlib>
#include <thread>
#include <cstring>
#include "ringbufferbase.tcc"
#include "shm.hpp"
#include "ringbuffertypes.hpp"
#include "SystemClock.tcc"

extern SystemClock< System > *system_clock;


namespace Monitor{
   struct QueueData 
   {
      uint64_t items_arrived;
      uint64_t items_departed;
      size_t   item_unit;
   };
}

template < class T, 
           RingBufferType type = RingBufferType::Normal, 
           bool monitor = false,
           size_t SIZE = 0 >  class RingBuffer : 
               public RingBufferBase< T, type >
{
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBuffer( const size_t n ) : RingBufferBase< T, type >()
   {
      (this)->data = new Buffer::Data<T, type >( n );
   }

   virtual ~RingBuffer()
   {
      delete( (this)->data );
   }

};

template< class T > class RingBuffer< T, 
                                      RingBufferType::Normal,
                                      true /* monitor */,
                                      0 > : 
      public RingBufferBase< T, RingBufferType::Normal >
{
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBuffer( const size_t n ) : RingBufferBase< T, RingBufferType::Normal >()
   {
      (this)->data = new Buffer::Data<T, 
                                      RingBufferType::Normal >( n );
   //   (this)->monitor = new std::thread( 
   }

   virtual ~RingBuffer()
   {
      monitor->join();
      delete( monitor );
      delete( (this)->data );
   }

private:
   static void monitor_thread( Buffer::Data<T, RingBufferType::Normal > *buffer,
                               volatile bool          &term,
                               Monitor::QueueData     &data )
   {
      
   }
   std::thread    *monitor;
   volatile bool  term;
};

/** 
 * SHM 
 */
template< class T, size_t SIZE > class RingBuffer< T, 
                                                   RingBufferType::SHM, 
                                                   false, 
                                                   SIZE > : 
   public RingBufferBase< T, RingBufferType::SHM >
{
public:
   RingBuffer( const std::string key,
               Direction         dir,
               bool              multi_threaded_init = true,
               const size_t      alignment = 8 ) : 
               RingBufferBase< T, RingBufferType::SHM >(),
                                              shm_key( key )
   {
      if( alignment % sizeof( void * ) != 0 )
      {
         std::cerr << "Alignment must be a multiple of ( " << 
            sizeof( void * ) << " ), reset and try again.";
         exit( EXIT_FAILURE );
      }
      /** calc total size **/
      total_size = sizeof(  Buffer::Data< T, RingBufferType::SHM , SIZE > );
      
      /** store already set to nullptr **/
      try
      {
         (this)->data = (Buffer::Data< T, RingBufferType::SHM>*) 
                         SHM::Init( 
                         key.c_str(),
                         total_size );
      }
      catch( bad_shm_alloc &ex )
      {
         try
         {
            (this)->data = (Buffer::Data< T, 
                                          RingBufferType::SHM>*) 
                                             SHM::Open( key.c_str() );
         }
         catch( bad_shm_alloc &ex2 )
         {
            std::cerr << ex2.what() << "\n";
            exit( EXIT_FAILURE );
         }
      }
     
      assert( (this)->data );

      {
         Pointer temp( SIZE );
         /**
          * Pointer has all static functions and some data members we need
          * to initialize like they are in the constructor, so memcopying will
          * work just fine
          */
         std::memcpy( &(this)->data->read_pt,    &temp, sizeof( Pointer ) );
         std::memcpy( &(this)->data->write_pt,   &temp, sizeof( Pointer ) );
      }
      (this)->data->max_cap = SIZE;
      
      switch( dir )
      {  
         case( Direction::Producer ):
            (this)->data->cookie.a = 0x1337;
            break;
         case( Direction::Consumer ):
            (this)->data->cookie.b = 0x1337;
            break;
         default:
            std::cerr << "Unknown direction (" << dir << "), exiting!!\n";
            exit( EXIT_FAILURE );
            break;
      }
      
      while( multi_threaded_init && (this)->data->cookie.a != (this)->data->cookie.b )
      {
         std::this_thread::yield();
      }
      /* theoretically we're all set */
   }

   virtual ~RingBuffer()
   {
      SHM::Close( shm_key.c_str(),
                  (void*) (this)->data,
                  total_size,
                  true,
                  true );
   }

protected:
   const  std::string shm_key;
   size_t total_size;
};

#endif /* END _RINGBUFFER_TCC_ */
