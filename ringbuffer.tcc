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
#include <cstddef>

#include "ringbufferbase.tcc"
#include "ringbuffertypes.hpp"



template < class T, 
           Type::RingBufferType type = Type::Heap >  class RingBuffer : 
               public RingBufferBase< T, type >
{
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBuffer( const std::size_t n, const std::size_t align = 16 ) : 
      RingBufferBase< T, type >()
   {
      (this)->data = new Buffer::Data<T, type >( n, align );
   }

   virtual ~RingBuffer()
   {
      delete( (this)->data );
   }

   /**
    * make_new_fifo - builder function to dynamically
    * allocate FIFO's at the time of execution.  The
    * first two parameters are self explanatory.  The
    * data ptr is a data struct that is dependent on the
    * type of FIFO being built.  In there really is no
    * data necessary so it is expacted to be set to nullptr
    * @param   n_items - std::size_t
    * @param   align   - memory alignment
    * @return  FIFO*
    */
   static FIFO* make_new_fifo( std::size_t n_items,
                               std::size_t align,
                               void *data )
   {
      assert( data == nullptr );
      return( new RingBuffer< T, Type::Heap >( n_items, align ) ); 
   }

};




/** 
 * SharedMemory 
 */
template< class T > class RingBuffer< T, 
                                      Type::SharedMemory > :
                            public RingBufferBase< T, Type::SharedMemory >
{
public:
   RingBuffer( const std::size_t      nitems,
               const std::string key,
               Direction         dir,
               const std::size_t      alignment = 16 ) : 
               RingBufferBase< T, Type::SharedMemory >(),
                                              shm_key( key )
   {
      (this)->data = 
         new Buffer::Data< T, 
                           Type::SharedMemory >( nitems, key, dir, alignment );
      assert( (this)->data != nullptr );
   }

   virtual ~RingBuffer()
   {
      delete( (this)->data );      
   }
  
   struct Data
   {
      const std::string key;
      Direction   dir;
   };

   /**
    * make_new_fifo - builder function to dynamically
    * allocate FIFO's at the time of execution.  The
    * first two parameters are self explanatory.  The
    * data ptr is a data struct that is dependent on the
    * type of FIFO being built.  In there really is no
    * data necessary so it is expacted to be set to nullptr
    * @param   n_items - std::size_t
    * @param   align   - memory alignment
    * @return  FIFO*
    */
   static FIFO* make_new_fifo( std::size_t n_items,
                               std::size_t align,
                               void *data )
   {
      auto *data_ptr( reinterpret_cast< Data* >( data ) );
      return( new RingBuffer< T, Type::SharedMemory>( n_items, 
                                                              data_ptr->key,
                                                              data_ptr->dir,
                                                              align ) ); 
   }

protected:
   const  std::string shm_key;
};

#endif /* END _RINGBUFFER_TCC_ */
