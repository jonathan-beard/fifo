/**
 * ringbufferinfinite.tcc - 
 * @author: Jonathan Beard
 * @version: Sun Sep  7 07:39:56 2014
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
#ifndef _RINGBUFFERINFINITE_TCC_
#define _RINGBUFFERINFINITE_TCC_  1

template < class T > class RingBufferBase< T, RingBufferType::Infinite > : public FIFO
{
public:
   /**
    * RingBuffer - default constructor, initializes basic
    * data structures.
    */
   RingBufferBase() : FIFO(),
                      data( nullptr ),
                      allocate_called( false ),
                      write_finished( false )
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
   virtual std::size_t   size()
   {
      return( 1 );
   }

   virtual RBSignal get_signal() 
   {
#if 0   
      /** 
       * there are two signalling paths, the one 
       * we'll give the highest priority to is the 
       * asynchronous one.
       */
      const auto signal_queue( data->store[ 0 ].signal );
      /**
       * TODO, fix this
       */
      const auto signal_local( (this)->signal_a );
      if( signal_local == RBSignal::NONE )
      {
         return( signal_queue );
      }
      /** there must be something in the local signal **/
      //(this)->signal = RBSignal::NONE;
#endif      
      return( RBSignal::NONE );
   }

   virtual bool send_signal( const RBSignal &signal )
   {
      //(this)->signal = signal;
      return( true );
   }

   /**
    * space_avail - returns the amount of space currently
    * available in the queue.  This is the amount a user
    * can expect to write without blocking
    * @return  size_t
    */
   virtual std::size_t   space_avail()
   {
      return( data->max_cap );
   }

  
   /**
    * capacity - returns the capacity of this queue which is 
    * set at compile time by the constructor.
    * @return size_t
    */
   virtual std::size_t   capacity() const
   {
      return( data->max_cap );
   }

   

   /**
    * push - releases the last item allocated by allocate() to
    * the queue.  Function will imply return if allocate wasn't
    * called prior to calling this function.
    * @param signal - const RBSignal signal, default: NONE
    */
   void push( const RBSignal signal = RBSignal::NONE )
   {
      if( ! (this)->allocate_called ) return;
      data->signal[ 0 ].sig = signal;
      write_stats.count++;
      (this)->allocate_called = false;
   }

   /**
    * push - This version won't write anything, it'll
    * increment the counter and simply return;
    * @param   item, T
    */
   void  push( T &item, const RBSignal signal = RBSignal::NONE )
   {
      data->store [ 0 ].item  = item;
      /** a bit awkward since it gives the same behavior as the actual queue **/
      data->signal[ 0 ].sig  = signal;
      write_stats.count++;
   }

   /**
    * insert - insert a range of items into the queue.
    * @param   begin - start iterator
    * @param   end   - ending iterator
    * @param   signal - const RBSignal, set if you want to send a signal
    */
   template< class iterator_type >
   void insert( iterator_type begin, 
                iterator_type end, 
                const RBSignal signal = RBSignal::NONE )
   {
      while( begin != end )
      {
         data->store[ 0 ].item = (*begin);
         begin++;
         write_stats.count++;
      }
      data->signal[ 0 ].sig = signal;
   }
 

   /**
    * pop - This version won't return any useful data,
    * its just whatever is in the buffer which should be zeros.
    * @return  T, item read.  It is removed from the
    *          q as soon as it is read
    */
   void pop( T &item, RBSignal *signal = nullptr )
   {
      item  = data->store[ 0 ].item;
      if( signal != nullptr )
      {
         *signal = data->signal[ 0 ].sig;
      }
      read_stats.count++;
   }
  
   /**
    * pop_range - dummy function version of the real one above
    * sets the input array to whatever "dummy" data has been
    * passed into the buffer, often real data.  In most cases
    * this will allow the application to work as it would with
    * correct inputs even if the overall output will be junk.  This
    * enables correct measurement of the arrival and service rates.
    * @param output - std:;array< T, N >*
    */
   template< size_t N >
   void  pop_range( 
      std::array< T, N > &output, 
      std::array< RBSignal, N > *signal = nullptr )
   {
      if( signal != nullptr )
      {
         for( size_t i( 0 ); i < N; i++ )
         {
            output[ i ]     = data->store [ 0 ].item;
            (*signal)[ i ]  = data->signal[ 0 ].sig;
         }
      }
      else
      {
         for( size_t i( 0 ); i < N; i++ )
         {
            output[ i ]    = data->store[ 0 ].item;
         }
      }
   }


   /**
    * peek() - look at a reference to the head of the
    * the ring buffer.  This doesn't remove the item, but it 
    * does give the user a chance to take a look at it without
    * removing.
    * @return T&
    */
   T& peek(  RBSignal *signal = nullptr )
   {
      T &output( data->store[ 0 ].item );
      if( signal != nullptr )
      {
         *signal = data->signal[  0  ].sig;
      }
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

   void get_zero_read_stats( Blocked &copy )
   {
      copy.all       = read_stats.all;
      read_stats.all = 0;
   }

   void get_zero_write_stats( Blocked &copy )
   {
      copy.all       = write_stats.all;
      write_stats.all = 0;
   }

protected:
   //TODO, come back to here
   /**
    * local_allocate - get a reference to an object of type T at the 
    * end of the queue.  Should be released to the queue using
    * the push command once the calling thread is done with the 
    * memory.
    * @return T&, reference to memory location at head of queue
    */
   T& local_allocate()
   {
      (this)->allocate_called = true;
      return( data->store[ 0 ].item );
   }
   /** go ahead and allocate a buffer as a heap, doesn't really matter **/
   Buffer::Data< T, RingBufferType::Heap >      *data;
   /** note, these need to get moved into the data struct **/
   volatile Blocked                             read_stats;
   volatile Blocked                             write_stats;
   
   volatile bool                                allocate_called;
   volatile bool                                write_finished;
};
#endif /* END _RINGBUFFERINFINITE_TCC_ */
