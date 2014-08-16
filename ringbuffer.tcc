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


/**
 * RingBuffer - template specializationf or Heap allocated thread
 * safe ringbuffer (designed to be used by one producer and one 
 * consumer thread).  Underlying implementation depends on a 
 * cache coherent multi-core system (shouldn't be a problem with
 * almost every modern system.
 * @templateparam T - type for queue to contain
 */
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
      (this)->data = nullptr;
   }

};




/** 
 * RingBuffer - template specialization for use with SHM, 
 * thread safe for two threads (one producer and one consumer)
 * to use lock free with a cache coherent multi-core processor
 * system.
 * @templateparam T - type or class for queue to contain
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
      (this)->data = nullptr;
   }

protected:
   const  std::string shm_key;
};


/**
 * TCP w/ multiplexing - not yet fully implemented, contemplating 
 * SSL ephemeral key implementation and how to unify mulitplexing
 * with multiplexing for above queues as well between common threads.
 * @templateparam T - type or class for queue to contain
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
