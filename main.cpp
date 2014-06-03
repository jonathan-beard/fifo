#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <unistd.h>
#include "ringbuffer.tcc"
#include "SystemClock.tcc"
#include <array>
struct Data
{
   Data( size_t send ) : send_count(  send )
   {}
   size_t                 send_count;
} data( 1e6 );


//#define USESharedMemory 1
#define USELOCAL 1
#define BUFFSIZE 1000000
#define MONITOR 1

#ifdef USESharedMemory
typedef RingBuffer< int64_t, RingBufferType::SharedMemory, BUFFSIZE > TheBuffer;
#elif defined USELOCAL
typedef RingBuffer< int64_t /* buffer type */,
                    RingBufferType::Heap/* allocation type */,
                    true /* turn on monitoring */ >  TheBuffer;
#endif


Clock *system_clock = new SystemClock< Cycle >;


void
producer( Data &data, TheBuffer &buffer )
{
   std::cout << "Producer thread starting!!\n";
   size_t current_count( 0 );
   const double service_time( 100.0e-6 );
   while( current_count++ < data.send_count )
   {
      buffer.push_back( current_count );
      const auto stop_time( system_clock->getTime() + service_time );
      while( system_clock->getTime() < stop_time );
   }
   buffer.push_back( -1 );
   std::cout << "Producer thread finished sending!!\n";
   return;
}

void 
consumer( Data &data , TheBuffer &buffer )
{
   std::cout << "Consumer thread starting!!\n";
   size_t   current_count( 0 );
   const double service_time( 50.0e-6 );
   while( true )
   {
      const auto sentinel( buffer.pop() );
      if( sentinel == -1 )
      {
         break;
      }
      current_count++;
      const auto stop_time( system_clock->getTime() + service_time );
      while( system_clock->getTime() < stop_time );
   }
   std::cout << "Received: " << current_count << "\n";
   return;
}


int 
main( int argc, char **argv )
{
#ifdef USESharedMemory
   char shmkey[ 256 ];
   SharedMemory::GenKey( shmkey, 256 );
   std::string key( shmkey );
   
   RingBuffer<int64_t, 
              RingBufferType::SharedMemory, 
              BUFFSIZE > buffer_a( key, 
                                   Direction::Producer, 
                                   false);
   RingBuffer<int64_t, 
              RingBufferType::SharedMemory, 
              BUFFSIZE > buffer_b( key, 
                                   Direction::Consumer, 
                                   false);

   std::thread a( producer, 
                  std::ref( data ), 
                  std::ref( buffer_a ) );

   std::thread b( consumer, 
                  std::ref( data ), 
                  std::ref( buffer_b ) );

   
#elif defined USELOCAL
   TheBuffer buffer( BUFFSIZE );
   const auto start_time( system_clock->getTime() );
   std::thread a( producer, 
                  std::ref( data ), 
                  std::ref( buffer ) );

   std::thread b( consumer, 
                  std::ref( data ), 
                  std::ref( buffer ) );
#endif
   a.join();
   b.join();
   buffer.monitor_off();
   const auto end_time( system_clock->getTime() );
#if MONITOR
   auto &monitor_data( buffer.getQueueData() );
   Monitor::QueueData::print( monitor_data, Monitor::QueueData::MB, std::cout ) << "\n";
#endif
   std::cerr << "Execution Time: " << (end_time - start_time ) << " seconds\n";
   return( EXIT_SUCCESS );
}
