/**
 * fifo.hpp - 
 * @author: Jonathan Beard
 * @version: Thu Sep  4 12:59:45 2014
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
#ifndef _FIFO_HPP_
#define _FIFO_HPP_  1
class FIFO
{
public:
   /**
    * FIFO - default constructor for base class for 
    * all subsequent ringbuffers.
    */
   FIFO() = default;
   
   /**
    * ~FIFO - default destructor
    */
   virtual ~FIFO() = default;

   /**
    * size - returns the current size of this FIFO
    * @return  std::size_t
    */
   virtual std::size_t size() = 0;

   /**
    * get_signal - used to get asynchronous signals
    * from the queue, the depth of each signalling
    * queue is implementation specific but must be 
    * at least (1).
    * @return  RBSignal, default is NONE
    */
   virtual RBSignal get_signal() = 0;

   /**
    * send_signal - used to send asynchronous signals
    * from the queue, the depth of each signalling
    * queue is implementation specific but must be
    * at least (1).
    * @param   signal - const RBSignal reference
    * @return  bool   - true if signal was sent
    */
   virtual bool send_signal( const RBSignal &signal ) = 0;

   /**
    * space_avail - convenience function to get the current
    * space available in the FIFO, could otherwise be calculated
    * by taking the capacity() - size().
    * @return std::size_t
    */
   virtual std::size_t space_avail() = 0;

   /**
    * capacity - returns the set maximum capacity of the 
    * FIFO.
    * @return std::size_t
    */
   virtual std::size_t capacity() = 0;

   /**
    * allocate - returns a reference to a writeable 
    * member at the tail of the FIFO.  You must have 
    * a subsequent call to push in order to release
    * this object to the FIFO once it is written.
    * @return  T&
    */
   template < class T > T& allocate()
   {
      void *ptr( std::nullptr );
      /** call blocks till an element is available **/
      local_allocate( &ptr );
      return( *( reinterpret_cast< T* >( ptr ) ) );
   }

   /**
    * releases the last item allocated by allocate() to the 
    * queue.  Function will simply return if allocate wasn't
    * called prior to calling this function.
    * @param   signal - cons tRBSignal signal, default: NONE
    */
   virtual push( const RBSignal signal = RBSignal::NONE ) = 0;


   /**
    * push - function which takes an object of type T and a 
    * signal, makes a copy of the object using the copy 
    * constructor and passes it to the FIFO along with the
    * signal which is guaranteed to be delivered at the 
    * same time as the object (if of course the receiving 
    * object is responding to signals).
    * @param   item -  T&
    * @param   signal -  RBSignal, default RBSignal::NONE
    */
   template < class T > 
   void push( T &item, const RBSignal signal = RBSignal::NONE )
   {
      void *ptr( (void*) &item );
      /** call blocks till element is written and released to queue **/
      local_push( ptr, signal );
      return;
   }

   /**
    * insert - inserts the range from begin to end in the FIFO,
    * blocks until space is available.  If the range is greater
    * than the space available it'll simply block and add items
    * as space becomes available.  There is the implicit assumption
    * that another thread is consuming the data, so eventually there 
    * will be room.
    * @param   begin - iterator_type, iterator to begin of range
    * @param   end   - iterator_type, iterator to end of range
    * @param   signal - RBSignal, default RBSignal::NONE
    */
   template< class iterator_type >
   void insert(   iterator_type begin,
                  iterator_type end,
                  const RBSignal signal = RBSignal::NONE )
   {
      void *begin_ptr( (void*)&begin );
      void *end_ptr  ( (void*)&end   );
      local_insert( begin_ptr, end_ptr, signal );
      return;
   }
   
   /**
    * pop - pops the head of the queue.  If the receiving
    * object wants to watch use the signal, then the signal
    * parameter should not be null.
    * @param   item - T&
    * @param   signal - RBSignal
    */
   template< class T >
   void pop( T &item, RBSignal *signal = std::nullptr )
   {
      void *ptr( (void*)&item );
      local_pop( ptr, signal );
      return;
   }
  
   /**
    * pop_range - pops a range and returns it as a std:;array.  The
    * exact range to be popped is specified as a template parameter.
    * @param   output - std::array< T, N >&
    * @param   signal - std::array< RBSignal, N >*
    */
   template< std::size_t N >
   void pop_range( std::array< T, N > &output,
                   std::array< RBSignal, N > *signal = std::nullptr )
   {

   }

protected:
   /** 
    * local_allocate - in order to get this whole thing
    * to work with multiple "ports" contained within the
    * same container we have to erase the type and then 
    * put it back.  To do this the template function
    * gives a void ptr mem address which is given the 
    * location to the head of the queue.  Once returned
    * the main allocate function reinterprets this as
    * the proper object type
    * @param   ptr - void **
    */
   virtual void local_allocate( void **ptr ) = 0;
   
   /**
    * local_push - pushes the object reference by the void
    * ptr and pushes it to the FIFO with the associated 
    * signal.  Once this function returns, the value is 
    * copied according to the objects copy constructor, 
    * which should be set up to "deep" copy the object
    * @param   ptr - void* 
    * @param   signal - RBSignal reference
    */
   virtual void local_push( void *ptr, const RBSignal &signal ) = 0;

   virtual void local_insert( void *ptr_begin, 
                              void *ptr_end, 
                              const RBSignal &signal ) = 0;
};
#endif /* END _FIFO_HPP_ */
