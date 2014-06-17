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
#include "signalvars.hpp"

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

   void  send_signal( const RBSignal sig )
   {
      signal_mask = sig;
   }
   
   RBSignal get_signal()
   {
      return( signal_mask ); 
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
    * push- writes a single item to the queue, blocks
    * until there is enough space.
    * @param   item, T
    */
   void  push( T &item, const RBSignal signal = RBSignal::RBNONE )
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
            
#if  __x86_64 
	int64_t size = sizeof(T);
	unsigned char *srcp = (unsigned char *)&item;
	unsigned char *dstp = (unsigned char *)&(data->store[write_index].item);
	
	__asm__ volatile("\
			movq	%[in], %%rax		\n\
			movq	%[out], %%rbx		\n\
			jmp 	l64ctl			\n\
		loop64:					\n\
			movdqu	(%%rax), %%xmm0		\n\
			movdqu	16(%%rax), %%xmm1	\n\
			movdqu	32(%%rax), %%xmm2	\n\
			movdqu	48(%%rax), %%xmm3	\n\
			movdqu	%%xmm0, (%%rbx)		\n\
			movdqu	%%xmm1, 16(%%rbx)	\n\
			movdqu	%%xmm2, 32(%%rbx)	\n\
			movdqu	%%xmm3, 48(%%rbx)	\n\
			addq	$64, %%rax		\n\
			addq	$64, %%rbx		\n\
			subq	$64, %[SIZE]		\n\
			l64ctl:				\n\
			cmpq	$64, %[SIZE]		\n\
			jge	loop64			\n\
			jmp	l32ctl			\n\
		loop32:					\n\
			movq	(%%rax), %%mm0		\n\
			movq	8(%%rax), %%mm1		\n\
			movq	16(%%rax), %%mm2	\n\
			movq	24(%%rax), %%mm3	\n\
			movntq	%%mm0, (%%rbx)		\n\
			movntq	%%mm1, 8(%%rbx)		\n\
			movntq	%%mm2, 16(%%rbx)	\n\
			movntq	%%mm3, 24(%%rbx)	\n\
			addq	$32, %%rax		\n\
			addq	$32, %%rbx		\n\
			subq	$32, %[SIZE]		\n\
			l32ctl:				\n\
			cmpq	$32, %[SIZE]		\n\
			jge	loop32			\n\
			jmp 	l8ctl			\n\
		loop8:					\n\
			movq	(%%rax), %%mm0		\n\
			movntq	%%mm0, (%%rbx)		\n\
			addq	$8, %%rax		\n\
			addq	$8, %%rbx		\n\
			subq	$8, %[SIZE]		\n\
			l8ctl:				\n\
			cmpq	$8, %[SIZE]		\n\
			jge	loop8			\n\
			jmp 	l1ctl			\n\
		loop1:					\n\
			movb	(%%rax), %%al		\n\
			movb	%%al, (%%rbx)		\n\
			incq	%%rax			\n\
			incq	%%rbx			\n\
			decq	%[SIZE]			\n\
			l1ctl:				\n\
			cmpq	$1, %[SIZE]		\n\
			jge	loop1"
			:
			:
			[in] "g" (srcp), 
			[out] "g" (dstp),			
			[SIZE] "m" (size)
			:
			"mm0", "mm1", "mm2", "mm3", "mm4", 
			"mm5", "mm6", "mm7", "rax", "rbx", "xmm0");	
#else
      data->store[ write_index ].item     = item;
#endif

      
      data->store[ write_index ].signal   = signal;
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
   void insert(   iterator_type begin, 
                  iterator_type end, 
                  const RBSignal signal = RBSignal::RBNONE )
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
            data->store[ write_index ].item = (*begin);
            /** add signal to last el only **/
            if( begin == ( end - 1 ) )
            {
               data->store[ write_index ].signal = signal;
            }
            else
            {
               data->store[ write_index ].signal = RBSignal::RBNONE;
            }
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
         //if( read_stats.blocked == 0 )
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
      Buffer::Element< T > output = data->store[ read_index ];
      signal_mask = output.signal;
      Pointer::inc( data->read_pt );
      read_stats.all++;
      return( output.item );
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
         (*output)[ i ] = data->store[ read_index ].item;
         Pointer::inc( data->read_pt );
         read_stats.count++;
      }
      /** the last element gets the signal **/
      signal_mask = output->back().signal;
      /** set this once and on both sides so it should work with SHM **/
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
      T &output( data->store[ read_index ].item );
      return( output );
   }


   /**
    * recycle - To be used in conjunction with peek().  Simply
    * removes the item at the head of the queue and discards them
    * @param range - const size_t, default range is 1
    */
   void recycle( const size_t range = 1 )
   {
      assert( range <= data->max_cap );
      Pointer::incBy( range, data->read_pt );
      read_stats.count += range;
   }

protected:
   Buffer::Data< T, type>      *data;
   volatile Blocked             read_stats;
   volatile Blocked             write_stats;
   volatile RBSignal            signal_mask;
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
      return( 0 );
   }
   
   void  send_signal( const RBSignal sig )
   {
      signal_mask = sig;
   }

   RBSignal get_signal() 
   {
      return( signal_mask );
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
    * push - This version won't write anything, it'll
    * increment the counter and simply return;
    * @param   item, T
    */
   void  push( T item, const RBSignal signal = RBSignal::RBNONE )
   {
      data->store[ 0 ].item   = item;
      /** a bit awkward since it gives the same behavior as the actual queue **/
      data->store[ 0 ].signal = signal;
      if(! (this)->used )
      {
         (this)->used = true;
      }
      write_stats.count++;
   }

   /**
    * insert - insert a range of items into the queue.
    * @param   begin - start iterator
    * @param   end   - ending iterator
    * @param   signal - const RBSignal, set if you want to send a signal
    */
   template< class iterator_type >
   void insert( iterator_type begin, iterator_type end, const RBSignal signal = RBSignal::RBNONE )
   {
      while( begin != end )
      {
         data->store[ 0 ].item = (*begin);
         begin++;
         write_stats.count++;
      }
      data->store[ 0 ].signal = signal;
      if(! (this)->used )
      {
         (this)->used = true;
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
      T output = data->store[ 0 ].item;
      (this)->signal_mask = data->store[ 0 ].signal;
      read_stats.count++;
      if(! (this)->used )
      {
         (this)->used = true;
      }
      return( output );
   }
   
   template< size_t N >
   std::array< T, N >* pop_range()
   {
      auto *output( new std::array< T, N >( ) );
      for( size_t i( 0 ); i < N; i++ )
      {
         (*output)[ i ] = data->store[ 0 ].item;
      }
      (this)->signal_mask = data->store[ 0 ].signal;
      if(! (this)->used )
      {
         (this)->used = true;
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
      T &output( data->store[ 0 ].item );
      return( output );
   }
   
   /**
    * recycle - remove ``range'' items from the head of the
    * queue and discard them.  Can be used in conjunction with
    * the peek operator.
    * @param   range - const size_t, default = 1
    */
   void recycle( const size_t range = 1 )
   {
      read_stats.count += range;
   }

protected:
   /** go ahead and allocate a buffer as a heap, doesn't really matter **/
   Buffer::Data< T, RingBufferType::Heap >      *data;
   volatile Blocked                             read_stats;
   volatile Blocked                             write_stats;
   volatile RBSignal                            signal_mask;
};
#endif /* END _RINGBUFFERBASE_TCC_ */
