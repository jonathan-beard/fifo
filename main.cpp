#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <unistd.h>
#include "ringbuffer.tcc"
#include "SystemClock.tcc"

struct Data{
   Data( size_t send ) : send_count(  send )
   {}

   size_t                 send_count;
} data( 1e6) ;


//#define USESHM 0
#define USELOCAL 1
#define BUFFSIZE 100000000
#define MONITOR 1

#ifdef USESHM
typedef RingBuffer< int64_t, RingBufferType::SHM, BUFFSIZE > TheBuffer;
#elif defined USELOCAL
typedef RingBuffer< int64_t, RingBufferType::Normal, true >  TheBuffer;
#endif


auto *system_clock = new SystemClock< System >;


void
producer( Data &data, TheBuffer &buffer )
{
   std::cout << "Producer thread starting!!\n";
   size_t current_count( 0 );
   const double service_time( 10.0e-6 );
   while( current_count++ < data.send_count )
   {
      buffer.blockingWrite( current_count );
      const auto stop_time( system_clock->getTime() + service_time );
      while( system_clock->getTime() < stop_time );
   }
   buffer.blockingWrite( -1 );
   std::cout << "Producer thread finished sending!!\n";
   return;
}

void 
consumer( Data &data , TheBuffer &buffer )
{
   std::cout << "Consumer thread starting!!\n";
   size_t   current_count( 0 );
   int64_t  sentinel( 0 );
   const double service_time( 5.0e-6 );
   while( true )
   {
      sentinel = buffer.blockingRead(); 
      const auto stop_time( system_clock->getTime() + service_time );
      while( system_clock->getTime() < stop_time );
      if( sentinel == -1 )
      {
         break;
      }
      current_count++;
   }
   std::cout << "Received: " << current_count << "\n";
   return;
}


int 
main( int argc, char **argv )
{
#ifdef USESHM
   char shmkey[ 256 ];
   SHM::GenKey( shmkey, 256 );
   std::string key( shmkey );
   
   RingBuffer<int64_t, 
              RingBufferType::SHM, 
              BUFFSIZE > buffer_a( key, 
                                   Direction::Producer, 
                                   false);
   RingBuffer<int64_t, 
              RingBufferType::SHM, 
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

   std::thread a( producer, 
                  std::ref( data ), 
                  std::ref( buffer ) );

   std::thread b( consumer, 
                  std::ref( data ), 
                  std::ref( buffer ) );
#endif
   a.join();
   b.join();

#if MONITOR
   auto &monitor_data( buffer.getQueueData() );

   double arrivalRate( (double) (monitor_data.max_arrived * monitor_data.item_unit ) 
                           / ( monitor_data.sample_frequency * monitor_data.arrived_samples ) );
   double departureRate( (double) ( monitor_data.max_departed * monitor_data.item_unit ) 
                           / (monitor_data.sample_frequency * monitor_data.departed_samples ) );
   std::cout << "Arrival Rate: " << arrivalRate << "\n"; 
   std::cout << "Departure Rate: " << departureRate << "\n";
   std::cout << "Rho: " << (arrivalRate / departureRate) << "\n";
   std::cout << "Mean Occupancy: " << monitor_data.total_occupancy / monitor_data.samples << "\n";
#endif
   return( EXIT_SUCCESS );
}
