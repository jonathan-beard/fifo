#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <unistd.h>
#include "ringbuffer.tcc"
#include "SystemClock.tcc"
#include <array>
#include <sstream>
#include <fstream>
#include "randomstring.tcc"
#include "signalvars.hpp"
#include <cassert>
#include <cinttypes>

#define MAX_VAL 100



struct Data
{
   Data( std::int64_t send ) : send_count(  send )
   {}
   std::int64_t          send_count;
} data( MAX_VAL );


//#define USESharedMemory 1
#define USELOCAL 1
#define BUFFSIZE 100

#ifdef USESharedMemory
typedef RingBuffer< std::int64_t, RingBufferType::SharedMemory, BUFFSIZE > TheBuffer;
#elif defined USELOCAL
typedef RingBuffer< std::int64_t          /* buffer type */,
                    RingBufferType::Heap  /* allocation type */,
                    true                  /* turn on monitoring */ >  TheBuffer;
#endif


Clock *system_clock = new SystemClock< Cycle >( 1 );


void
producer( Data &data, TheBuffer &buffer )
{
   std::int64_t current_count( 0 );
   const double service_time( 10.0e-6 );
   while( current_count++ < data.send_count )
   {
      auto &ref( buffer.allocate() );
      ref = current_count;
      buffer.push( /* current_count, */ 
         (current_count == data.send_count ? 
          RBSignal::RBEOF : RBSignal::NONE ) );
      const auto stop_time( system_clock->getTime() + service_time );
      while( system_clock->getTime() < stop_time );
   }
   return;
}

void 
consumer( Data &data , TheBuffer &buffer )
{
   std::int64_t   current_count( 0 );
   const double service_time( 5.0e-6 );
   RBSignal signal( RBSignal::NONE );
   while( signal != RBSignal::RBEOF )
   {
      buffer.pop( current_count, &signal );
      const auto stop_time( system_clock->getTime() + service_time );
      fprintf( stdout, "%" PRIi64 "\n", current_count );
      while( system_clock->getTime() < stop_time );
   }
   assert( current_count == MAX_VAL );
   return;
}

std::string test()
{
#ifdef USESharedMemory
   char shmkey[ 256 ];
   SharedMemory::GenKey( shmkey, 256 );
   std::string key( shmkey );
   
   RingBuffer< std::int64_t, 
               RingBufferType::SharedMemory, 
               BUFFSIZE > buffer_a( key, 
                                    Direction::Producer, 
                                    false);
   RingBuffer< std::int64_t, 
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
   std::thread a( producer, 
                  std::ref( data ), 
                  std::ref( buffer ) );

   std::thread b( consumer, 
                  std::ref( data ), 
                  std::ref( buffer ) );
#endif
   a.join();
   b.join();
   auto &monitor_data( buffer.getQueueData() );
   std::stringstream ss;
   Monitor::QueueData::print( monitor_data, Monitor::QueueData::Bytes, ss, true);
   return( ss.str() );
}


int 
main( int argc, char **argv )
{
   //RandomString< 50 > rs;
   //const std::string root( "/project/mercury/svardata/" );
   //const std::string root( "" );
   //std::ofstream ofs( root + rs.get() + ".csv" );
   //if( ! ofs.is_open() )
   //{
   //   std::cerr << "Couldn't open ofstream!!\n";
   //   exit( EXIT_FAILURE );
   //}
   int runs( 2 );
   while( runs-- )
   {
       std::cout << test() << "\n";
   }
   //ofs.close();
   if( system_clock != nullptr ) 
      delete( system_clock );
}

