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
//#define NICE 1

typedef std::uint32_t blocked_part_t;
typedef std::uint64_t blocked_whole_t;

/**
 * Blocked - simple data structure to combine the send count
 * and blocked flag in one simple structure.  Greatly improves
 * synchronization between cores.
 */
union Blocked{
   
   Blocked() : all( 0 )
   {}

   Blocked( volatile Blocked &other )
   {
      all = other.all;
   }

   struct{
      blocked_part_t  count;
      blocked_part_t  blocked;
   };
   blocked_whole_t    all;
} __attribute__ ((aligned( 8 )));

template < class T, 
           RingBufferType type > class RingBufferBase {
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBufferBase() : data( nullptr ),
                      signal_mask( 0 )
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
             * is a better way to fix this with atomic operations...
             * or on second thought benchmarking shows the atomic
             * operations slows the queue down drastically so, perhaps
             * this is in fact the best of all possible returns.
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

   void  send_signal( const uint64_t sig )
   {
      signal_mask = sig;
   }
   
   /**
    * space_avail - returns the amount of space currently
    * available in the queue.  This is the amount a user
    * can expect to write without blocking
    * @return  size_t
    */
    size_t   space_avail()
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
    * push_back - writes a single item to the queue, blocks
    * until there is enough space.
    * @param   item, T
    */
   void  push_back( T item )
   {
      while( space_avail() == 0 )
      {
#ifdef NICE      
         std::this_thread::yield();
#endif         
         if( write_stats.blocked == 0 )
         {   
            write_stats.blocked = 1;
         }
#if __x86_64 
         __asm__ volatile("\
           pause"
           :
           :
           : );
#endif           
      }
      const size_t write_index( Pointer::val( data->write_pt ) );
      data->store[ write_index ] = item;
      Pointer::inc( data->write_pt );
      write_stats.all++;
   }
   
   /**
    * insert - inserts the range from begin to end in the queue,
    * blocks until space is available.  If the range is greater than
    * available space on the queue then it'll simply add items as 
    * space becomes available.  There is the implicit assumption that
    * another thread is consuming the data, so eventually there will
    * be room.
    * @param   begin - iterator_type, iterator to begin of range
    * @param   end   - iterator_type, iterator to end of range
    */
   template< class iterator_type >
   void insert( iterator_type begin, iterator_type end )
   {
      while( begin != end )
      {
         if( space_avail() == 0 )
         {
#ifdef NICE
            std::this_thread::yield();
#endif
            if( write_stats.blocked == 0 )
            {
               write_stats.blocked = 1;
            }
         }
         else
         {
            const size_t write_index( Pointer::val( data->write_pt ) );
            data->store[ write_index ] = (*begin);
            Pointer::inc( data->write_pt );
            write_stats.all++;
            begin++;
         }
      }
   }

  
   /**
    * pop - read one item from the ring buffer,
    * will block till there is data to be read
    * @return  T, item read.  It is removed from the
    *          q as soon as it is read
    */
    T pop()
   {
      while( size() == 0 )
      {
#ifdef NICE      
         std::this_thread::yield();
#endif        
         if( read_stats.blocked == 0 )
         {   
            read_stats.blocked  = 1;
         }
#if __x86_64 
         __asm__ volatile("\
           pause"
           :
           :
           : );
#endif           
      }
      const size_t read_index( Pointer::val( data->read_pt ) );
      T output = data->store[ read_index ];
      Pointer::inc( data->read_pt );
      read_stats.all++;
      return( output );
   }

   /**
    * pop_range - pops a range and returns it as a std::array.  The
    * exact range to be popped is specified as a template parameter.
    * the static std::array was chosen as its a bit faster, however 
    * this might change in future implementations to a std::vector
    * or some other structure.
    */
   template< size_t N >
   std::array< T, N >*  pop_range()
   {
      while( size() < N )
      {
#ifdef NICE
         std::this_thread::yield();
#endif
         if( read_stats.blocked == 0 )
         {
            read_stats.blocked = 1;
         }
      }
      auto *output( new std::array< T, N >() );
      //TODO, this section could be optimized quite a bit
      for( size_t i( 0 ); i < N; i++ )
      {
         const size_t read_index( Pointer::val( data->read_pt ) );
         (*output)[ i ] = data->store[ read_index ];
         Pointer::inc( data->read_pt );
         read_stats.count++;
      }
      return( output );
   }


   /**
    * peek() - look at a reference to the head of the
    * ring buffer.  This doesn't remove the item, but it 
    * does give the user a chance to take a look at it without
    * removing.
    * @return T&
    */
    T& peek()
   {
      while( size() < 1 )
      {
#ifdef NICE      
         std::this_thread::yield();
#endif     
#if   __x86_64        
         __asm__ volatile("\
           pause"
           :
           :
           : );
#endif
      }
      const size_t read_index( Pointer::val( data->read_pt ) );
      T &output( data->store[ read_index ] );
      return( output );
   }

   void recycle()
   {
      Pointer::inc( data->read_pt );
      read_stats.count++;
   }

   void recycle_range( const size_t range )
   {
      assert( range <= data->max_cap ); 
      Pointer::incBy( range, data->read_pt );
      read_stats.count += range;
   }

protected:
   Buffer::Data< T, type>      *data;
   volatile Blocked                             read_stats;
   volatile Blocked                             write_stats;
   uint64_t signal_mask;
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
                        signal_mask( 0 )
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
      return( 0 );
   }
   
   void  send_signal( const uint64_t sig )
   {
      signal_mask = sig;
   }
   /**
    * space_avail - returns the amount of space currently
    * available in the queue.  This is the amount a user
    * can expect to write without blocking
    * @return  size_t
    */
    size_t   space_avail()
   {
      return( data->max_cap );
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
    * push_back - This version won't write anything, it'll
    * increment the counter and simply return;
    * @param   item, T
    */
   void  push_back( T item )
   {
      data->store[ 0 ] = item ;
      write_stats.count++;
   }

   template< class iterator_type >
   void insert( iterator_type begin, iterator_type end )
   {
      while( begin != end )
      {
         data->store[ 0 ] = (*begin);
         begin++;
         write_stats.count++;
      }
   }
 

   /**
    * pop - This version won't return any useful data,
    * its just whatever is in the buffer which should be zeros.
    * @return  T, item read.  It is removed from the
    *          q as soon as it is read
    */
   T pop()
   {
      T output = data->store[ 0 ];
      read_stats.count++;
      return( output );
   }
   
   template< size_t N >
   std::array< T, N >* pop_range()
   {
      auto *output( new std::array< T, N >( ) );
      for( size_t i( 0 ); i < N; i++ )
      {
         (*output)[ i ] = data->store[ 0 ];
      }
      return( output );
   }


   /**
    * peek() - look at a reference to the head of the
    * the ring buffer.  This doesn't remove the item, but it 
    * does give the user a chance to take a look at it without
    * removing.
    * @return T&
    */
    T& peek()
   {
      T &output( data->store[ 0 ] );
      return( output );
   }
   
   void recycle()
   {
      read_stats.count++;
   }

   void recycle_range( const size_t range )
   {
      read_stats.count += range;
   }

protected:
   /** go ahead and allocate a buffer as a heap, doesn't really matter **/
   Buffer::Data< T, RingBufferType::Heap >      *data;
   volatile Blocked                             read_stats;
   volatile Blocked                             write_stats;
   uint64_t signal_mask;
};
#endif /* END _RINGBUFFERBASE_TCC_ */
