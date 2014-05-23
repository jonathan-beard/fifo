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
#include <thread>
#include <cstring>
#include "pointer.hpp"
#include "ringbuffertypes.hpp"
#include "bufferdata.tcc"


template < class T, 
           RingBufferType type,
           bool monitor = false > class RingBufferBase {
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBufferBase() : data( nullptr )
   {
   }
   
   virtual ~RingBufferBase()
   {
   }


   /**
    * size - as you'd expect it returns the number of 
    * items currently in the queue.
    * @return size_t
    */
   size_t   size()
   {
      const size_t wpt( Pointer::val( data->write_pt ) ), 
                   rpt( Pointer::val( data->read_pt  ) );
      if( wpt == rpt )
      {
         const size_t wrap_write( Pointer::wrapIndicator( data->write_pt  ) ),
                      wrap_read(  Pointer::wrapIndicator( data->read_pt   ) );

         if( wrap_read < wrap_write )
         {
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
         std::this_thread::yield();
      }
      const size_t write_index( Pointer::val( data->write_pt ) );
      data->store[ write_index ] = item;
      Pointer::inc( data->write_pt );
   }

  
   /**
    * blockingRead - read one item from the ring buffer,
    * will block till there is data to be read
    * @return  T, item read.  It is removed from the
    *          q as soon as it is read
    */
   T blockingRead()
   {
      while( size() < 1 )
      {
         std::this_thread::yield();
      }
      const size_t read_index( Pointer::val( data->read_pt ) );
      T output = data->store[ read_index ];
      Pointer::inc( data->read_pt );
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
         std::this_thread::yield();
      }
      const size_t read_index( Pointer::val( data->read_pt ) );
      T &output( data->store[ read_index ] );
      return( output );
   }

protected:
   Buffer::Data< T, type>      *data;
};
#endif /* END _RINGBUFFERBASE_TCC_ */
