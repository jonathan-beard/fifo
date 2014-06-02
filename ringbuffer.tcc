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
 * 
 * Notes:  When using monitoring, the cycle counter is the most accurate for
 * Linux / Unix platforms.  It is really not suited to OS X's mach_absolute_time()
 * function since it is so slow relative to the movement of data, the 
 * results returned for high throughput systems are simply not accurate
 * on that platform.
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

extern Clock *system_clock;
const double sample_freq = 1e-8;

namespace Monitor
{
   struct QueueData 
   {
      /**
       * QueueData - basic constructor
       * @param sample_frequency - in seconds
       * @param nbytes - number of bytes in each queue item
       */
      QueueData( double sample_frequency, size_t nbytes ) :
         items_arrived( 0 ),
         max_arrived( 0 ),
         arrived_samples( 0 ),
         items_departed( 0 ),
         max_departed( 0 ),
         departed_samples( 0 ),
         total_occupancy( 0 ),
         item_unit( nbytes ),
         samples( 0 ),
         sample_frequency( sample_frequency )
      {
      }

      QueueData( const QueueData &other ) : item_unit( other.item_unit ),
                                            sample_frequency( other.sample_frequency )
      {
         items_arrived     = other.items_arrived;
         max_arrived       = other.max_arrived;
         arrived_samples   = other.arrived_samples;
         items_departed    = other.items_departed;
         max_departed      = other.max_departed;
         departed_samples  = other.departed_samples;
         total_occupancy   = other.total_occupancy;
         samples           = other.samples;
      }

      uint64_t    items_arrived;
      uint64_t    max_arrived;
      uint64_t    arrived_samples;
      uint64_t    items_departed;
      uint64_t    max_departed;
      uint64_t    departed_samples;
      uint64_t    total_occupancy;
      const size_t      item_unit;
      uint64_t    samples;
      const double sample_frequency;
   };
}

template < class T, 
           RingBufferType type = RingBufferType::Heap, 
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

template< class T, RingBufferType type > class RingBufferBaseMonitor : public
   RingBufferBase< T, type >
{
public:
   RingBufferBaseMonitor( const size_t n ) : RingBufferBase< T, type >(),
                                             monitor_data( sample_freq, sizeof( T ) ),
                                             monitor( nullptr ),
                                             term( false )
   {
      (this)->data = new Buffer::Data<T, 
                                      RingBufferType::Heap >( n );

      (this)->monitor = new std::thread( monitor_thread, 
                                         std::ref( *(this) ),
                                         std::ref( (this)->term ),
                                         std::ref( (this)->monitor_data ) );

   }

   virtual ~RingBufferBaseMonitor()
   {
      (this)->term = true;
      monitor->join();
      delete( monitor );
      monitor = nullptr;
      delete( (this)->data );
   }

   volatile Monitor::QueueData& 
      getQueueData()
   {
      return( monitor_data );
   }

protected:
   /**
    * monitor_thread - implements queue monitoring for arrival rate and
    * departure rate (service rate) from the queue.  Also enables mean 
    * queue occupancy monitoring.  Other functions could easily be added
    * as well, such as an all full counter, or a full histogram for each
    * queue position.
    * @param buffer - ring buffer of this type
    * @param term   - bool to stop the monitor thread
    * @param data   - state data to return to the process monitoring this queue
    */
   static void monitor_thread( RingBufferBaseMonitor< T, type >     &buffer,
                               volatile bool          &term,
                               volatile Monitor::QueueData     &data )
   {
      
      while( ! term )
      {
         const Blocked write_copy( buffer.write_stats );
         buffer.write_stats.all = 0;
         if( ! write_copy.blocked  )
         {
            data.max_arrived += write_copy.count;
            data.arrived_samples++;
         }

         const Blocked read_copy( buffer.read_stats );
         buffer.read_stats.all = 0;
         if( ! read_copy.blocked )
         {
            data.max_departed += read_copy.count;
            data.departed_samples++;
         }
         
         data.total_occupancy += buffer.size();
         data.samples         += 1;
         const auto stop_time( data.sample_frequency + system_clock->getTime() );
         while( system_clock->getTime() < stop_time )
         {
            if( term ) break;
            //needs to be fairly precisely timed so no yeilding here
         }
      }
   }
   
   volatile Monitor::QueueData monitor_data;
   std::thread       *monitor;
   volatile bool      term;
};

template< class T > class RingBuffer< T, 
                                      RingBufferType::Heap,
                                      true /* monitor */ > :
      public RingBufferBaseMonitor< T, RingBufferType::Heap >
{
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBuffer( const size_t n ) : RingBufferBaseMonitor< T, RingBufferType::Heap >( n )
   {
      /** nothing really to do **/
   }
   
   virtual ~RingBuffer()
   {
      /** nothing really to do **/
   }
};

/** specialization for dummy one **/
template< class T > class RingBuffer< T, 
                                      RingBufferType::Infinite,
                                      true /* monitor */ > :
      public RingBufferBaseMonitor< T, RingBufferType::Infinite >
{
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBuffer( const size_t n ) : RingBufferBaseMonitor< T, RingBufferType::Infinite >( 1 )
   {
   }
   virtual ~RingBuffer()
   {
      /** nothing really to do **/
   }
};

/** 
 * SharedMemory 
 */
template< class T, size_t SIZE > class RingBuffer< T, 
                                                   RingBufferType::SharedMemory, 
                                                   false, 
                                                   SIZE > : 
   public RingBufferBase< T, RingBufferType::SharedMemory >
{
public:
   RingBuffer( const std::string key,
               Direction         dir,
               bool              multi_threaded_init = true,
               const size_t      alignment = 8 ) : 
               RingBufferBase< T, RingBufferType::SharedMemory >(),
                                              shm_key( key )
   {
      if( alignment % sizeof( void * ) != 0 )
      {
         std::cerr << "Alignment must be a multiple of ( " << 
            sizeof( void * ) << " ), reset and try again.";
         exit( EXIT_FAILURE );
      }
      /** calc total size **/
      total_size = 
         sizeof(  Buffer::Data< T, RingBufferType::SharedMemory , SIZE > );
      
      /** store already set to nullptr **/
      try
      {
         (this)->data = (Buffer::Data< T, RingBufferType::SharedMemory>*) 
                         SHM::Init( 
                         key.c_str(),
                         total_size );
      }
      catch( bad_shm_alloc &ex )
      {
         try
         {
            (this)->data = (Buffer::Data< T, 
                                          RingBufferType::SharedMemory>*) 
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
