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
#include "sample.tcc"
#include "meansampletype.tcc"

extern Clock *system_clock;


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
            monitor( nullptr ),
            term( false )
   {
      (this)->data = new Buffer::Data<T, 
                                      RingBufferType::Heap >( n );

      /** add monitor types immediately after construction **/
      sample_master.registerSample( new MeanSampleType< T, type >() );

      (this)->monitor = new std::thread( Sample< T, type >::run, 
                                         std::ref( *(this)      /** buffer **/ ),
                                         std::ref( (this)->term /** term bool **/ ),
                                         std::ref( (this)->sample_master ) );

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

   std::ostream&
   printQueueData( std::ostream &stream )
   {
      stream << sample_master.printAllData( '\n' );
      return( stream );
   }
protected:
   std::thread       *monitor;
   volatile bool      term;
   Sample< T, type >  sample_master;
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
