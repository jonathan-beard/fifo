/**
 * ringbufferbase.tcc - 
 * @author: Jonathan Beard
 * @version: Thu May 15 09:06:52 2014
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
#ifndef _RINGBUFFERBASE_TCC_
#define _RINGBUFFERBASE_TCC_  1

#include <array>
#include <cstdlib>
#include <cassert>
#include <thread>
#include <cstring>
#include "pointer.hpp"
#include "ringbuffertypes.hpp"
#include "bufferdata.tcc"

/**
 * Note: there is a NICE define that can be uncommented
 * below if you want sched_yield called when waiting for
 * writes or blocking for space, otherwise blocking will
 * actively spin while waiting.
 */
#define NICE 1

template < class T, 
           RingBufferType type > class RingBufferBase {
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBufferBase() : data( nullptr ),
                      read_count( 0 ),
                      write_count( 0 )
   {
   }
   
   virtual ~RingBufferBase()
   {
      read_count = 0;
      write_count = 0;
   }


   /**
    * size - as you'd expect it returns the number of 
    * items currently in the queue.
    * @return size_t
    */
   size_t   size()
   {
      const auto   wrap_write( Pointer::wrapIndicator( data->write_pt  ) ),
                   wrap_read(  Pointer::wrapIndicator( data->read_pt   ) );
      const auto   wpt( Pointer::val( data->write_pt ) ), 
                   rpt( Pointer::val( data->read_pt  ) );
      if( wpt == rpt )
      {
         if( wrap_read < wrap_write )
         {
            return( data->max_cap );
         }
         else if( wrap_read > wrap_write )
         {
            /**
             * TODO, this condition is momentary, however there
             * is a better way to fix this with atomic operations
             */
            return( data->max_cap );
         }
         else
         {
            return( 0 );
         }
      }
      else if( rpt < wpt )
      {
         return( wpt - rpt );
      }
      else if( rpt > wpt )
      {
         return( data->max_cap - rpt + wpt ); 
      }
      return( 0 );
   }
   
   /**
    * spaceAvail - returns the amount of space currently
    * available in the queue.  This is the amount a user
    * can expect to write without blocking
    * @return  size_t
    */
    size_t   spaceAvail()
   {
      return( data->max_cap - size() );
   }
  
   /**
    * capacity - returns the capacity of this queue which is 
    * set at compile time by the constructor.
    * @return size_t
    */
   size_t   capacity() const
   {
      return( data->max_cap );
   }

   /**
    * blockingWrite - writs a single item to the queue, blocks
    * until there is enough space.
    * @param   item, T
    */
   void  blockingWrite( T item )
   {
      while( spaceAvail() == 0 )
      {
#ifdef NICE      
         std::this_thread::yield();
#endif         
      }
      const size_t write_index( Pointer::val( data->write_pt ) );
      data->store[ write_index ] = item;
      Pointer::inc( data->write_pt );
      write_count++;
   }

  
   /**
    * blockingRead - read one item from the ring buffer,
    * will block till there is data to be read
    * @return  T, item read.  It is removed from the
    *          q as soon as it is read
    */
    T blockingRead()
   {
      while( size() == 0 )
      {
#ifdef NICE      
         std::this_thread::yield();
#endif        
      }
      const size_t read_index( Pointer::val( data->read_pt ) );
      T output = data->store[ read_index ];
      Pointer::inc( data->read_pt );
      read_count++;
      return( output );
   }


   /**
    * blockingPeek() - look at a reference to the head of the
    * the ring buffer.  This doesn't remove the item, but it 
    * does give the user a chance to take a look at it without
    * removing.
    * @return T&
    */
    T& blockingPeek()
   {
      while( size() < 1 )
      {
#ifdef NICE      
         std::this_thread::yield();
#endif         
      }
      const size_t read_index( Pointer::val( data->read_pt ) );
      T &output( data->store[ read_index ] );
      return( output );
   }

protected:
   Buffer::Data< T, type>      *data;
   std::uint64_t               read_count;
   std::uint64_t               write_count;
};


/**
 * Infinite / Dummy  specialization 
 */
template < class T > class RingBufferBase< T, RingBufferType::Infinite >
{
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBufferBase() : data( nullptr ),
                      read_count( 0 ),
                      write_count( 0 )
   {
   }
   
   virtual ~RingBufferBase()
   {
      read_count = 0;
      write_count = 0;
   }


   /**
    * size - as you'd expect it returns the number of 
    * items currently in the queue.
    * @return size_t
    */
   size_t   size()
   {
      return( 0 );
   }
   
   /**
    * spaceAvail - returns the amount of space currently
    * available in the queue.  This is the amount a user
    * can expect to write without blocking
    * @return  size_t
    */
    size_t   spaceAvail()
   {
      return( data->max_cap - size() );
   }
  
   /**
    * capacity - returns the capacity of this queue which is 
    * set at compile time by the constructor.
    * @return size_t
    */
   size_t   capacity() const
   {
      return( data->max_cap );
   }

   /**
    * blockingWrite - This version won't write anything, it'll
    * increment the counter and simply return;
    * @param   item, T
    */
   void  blockingWrite( T item )
   {
      write_count++;
   }

  
   /**
    * blockingRead - This version won't return any useful data,
    * its just whatever is in the buffer which should be zeros.
    * @return  T, item read.  It is removed from the
    *          q as soon as it is read
    */
    T blockingRead()
   {
      const size_t read_index( 1 );
      T output = data->store[ read_index ];
      read_count++;
      return( output );
   }


   /**
    * blockingPeek() - look at a reference to the head of the
    * the ring buffer.  This doesn't remove the item, but it 
    * does give the user a chance to take a look at it without
    * removing.
    * @return T&
    */
    T& blockingPeek()
   {
      const size_t read_index( 1 );
      T &output( data->store[ read_index ] );
      return( output );
   }

protected:
   /** go ahead and allocate a buffer **/
   Buffer::Data< T, RingBufferType::Normal>      *data;
   std::uint64_t               read_count;
   std::uint64_t               write_count;
};
#endif /* END _RINGBUFFERBASE_TCC_ */
