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
#include <cstdint>
#include <vector>
#include <iostream>
#include <fstream>
#include <utility>

#include "ringbufferbase.tcc"
#include "ringbuffertypes.hpp"
#include "SystemClock.tcc"

extern Clock *system_clock;

/**
 * This include has all the code for the monitor.
 * It was removed to make this file a bit more 
 * readable.
 */
#include "monitor.hpp"

template < class T, 
           RingBufferType type = RingBufferType::Heap, 
           bool monitor = false >  class RingBuffer : 
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

template< class T, 
          RingBufferType type > class RingBufferBaseMonitor : 
            public RingBufferBase< T, type >
{
public:
   RingBufferBaseMonitor( const size_t n ) : 
            RingBufferBase< T, type >(),
            monitor_data( sizeof( T ) ),
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

   void  monitor_off()
   {
      (this)->term = true;
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
      bool arrival_started( false );
      switch( type )
      {
         case( RingBufferType::Heap ):
         {
            bool converged( false );
            auto prev_time( system_clock->getTime() ); 
            while( ! term )
            {
               const auto stop_time( 
                  data.resolution.curr_frame_width + system_clock->getTime() );
               while( system_clock->getTime() < stop_time  && ! term )
               {
#if __x86_64            
                  __asm__ volatile("\
                     pause"
                     :
                     :
                     : );
#endif               
               }
               const Blocked read_copy ( buffer.read_stats );
               const Blocked write_copy( buffer.write_stats );
               buffer.read_stats.all   = 0;
               buffer.write_stats.all  = 0;
               if( ! arrival_started )
               {
                  if( write_copy.count != 0 )
                  {
                     arrival_started = true;
                     std::this_thread::yield();
                     continue;
                  }
               }
               /**
                * if we're not blocked, and the server has actually started
                * and the end of data signal has not been received then 
                * record the throughput within this frame
                */
               if( write_copy.blocked == 0 && 
                     arrival_started  && ! buffer.write_finished ) 
               {
                  Monitor::frame_resolution::setBlockedStatus( data.resolution,
                                                      false );
                  if( converged )
                  {
                     data.arrival.items         += write_copy.count;
                     data.arrival.frame_count   += 1;
                  }
                  else
                  {
                     data.arrival.items         += 0;
                     data.arrival.frame_count   += 0;
                  }
               }
               else
               {
                  frame_resolution::setBlockedStatus( data.resolution,
                                                      true );
               }
               
               /**
                * if we're not blocked, and the server has actually started
                * and the end of data signal has not been received then 
                * record the throughput within this frame
                */
               if( read_copy.blocked == 0 )
               {
                  frame_resolution::setBlockedStatus( data.resolution,
                                                      false );
                  if( converged )
                  {
                     data.departure.items       += read_copy.count;
                     data.departure.frame_count += 1;
                  }
                  else
                  {
                     data.departure.items      += 0;
                     data.departure.frame_cunt += 0;
                  }
               }
               else
               {
                  Monitor::frame_resolution::setBlockedStatus( data.resolution,
                                                      true );
               }
               data.mean_occupancy.items        += buffer.size();
               data.mean_occupancy.frame_count  += 1;
               const auto total_time( system_clock->getTime() - prev_time );

               data.resolution.updateResolution( qd.resolution,
                                                 total_time );
               prev_time = total_time;
            }

            /** log **/
            //std::ofstream ofs( "/tmp/log.csv" );
            //if( ! ofs.is_open() )
            //{
            //   std::cerr << "Failed to open output log\n";
            //   exit( EXIT_FAILURE );
            //}
            //for( auto pair : loglist )
            //{
            //   ofs << pair.first << "," << pair.second << "\n";
            //}
            //ofs.close();
         }
         break;
         case( RingBufferType::Infinite ):
         {
            /**
             * set departed_samples and arrived_samples to 1 so
             * the multiplication above works out for the infinite
             * queue.
             */
            data.departed_samples = 1;
            data.arrived_samples  = 1;
            const auto start_time( system_clock->getTime() );
            while( ! term )
            {
#if __x86_64            
                  __asm__ volatile("\
                     pause"
                     :
                     :
                     : );
#endif               
               const auto end_time(   system_clock->getTime() );
               data.items_arrived   = buffer.write_stats.count;
               data.items_departed  = buffer.read_stats.count;
               /** set sample frequency to time diff **/
               data.sample_frequency = ( end_time - start_time );
            }
         }
         break;
         default:
            assert( false );
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
template< class T > class RingBuffer< T, 
                                      RingBufferType::SharedMemory, 
                                      false > :
                            public RingBufferBase< T, RingBufferType::SharedMemory >
{
public:
   RingBuffer( const size_t      nitems,
               const std::string key,
               Direction         dir,
               const size_t      alignment = 16 ) : 
               RingBufferBase< T, RingBufferType::SharedMemory >(),
                                              shm_key( key )
   {
      (this)->data = 
         new Buffer::Data< T, 
                           RingBufferType::SharedMemory >( nitems, key, dir, alignment );
      assert( (this)->data != nullptr );
   }

   virtual ~RingBuffer()
   {
      delete( (this)->data );      
   }

protected:
   const  std::string shm_key;
};


/**
 * TCP w/ multiplexing
 */
template <class T> class RingBuffer< T,
                                     RingBufferType::TCP,
                                     false /* no monitoring yet */ > :
                                       public RingBufferBase< T, RingBufferType::Heap >
{
public:
   RingBuffer( const size_t      nitems,
               const std::string dns_name,
               Direction         dir,
               const size_t      alignment = 16 ) : 
                  RingBufferBase< T, 
                                  RingBufferType::Heap >()
   {
      //TODO, fill in stuff here
   }

   virtual ~RingBuffer()
   {

   }

protected:
};
#endif /* END _RINGBUFFER_TCC_ */
