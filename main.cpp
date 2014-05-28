#include <cstdlib>
#include <iostream>
#include <thread>
#include <cstdint>
#include <unistd.h>
#include "ringbuffer.tcc"

struct Data{
   Data( size_t send ) : send_count(  send )
   {}

   size_t                 send_count;
} data( 1e7 ) ;


//#define USESHM 0
#define USELOCAL 1
#define BUFFSIZE 10000

#ifdef USESHM
typedef RingBuffer< int64_t, RingBufferType::SHM, BUFFSIZE > TheBuffer;
#elif defined USELOCAL
typedef RingBuffer< int64_t >                                TheBuffer;
#endif


void
producer( Data &data, TheBuffer &buffer )
{
   std::cout << "Producer thread starting!!\n";
   size_t current_count( 0 );
   while( current_count++ < data.send_count )
   {
      buffer.blockingWrite( current_count );
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
   while( true )
   {
      sentinel = buffer.blockingRead(); 
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
   RingBuffer< int64_t > buffer( BUFFSIZE );

   std::thread a( producer, 
                  std::ref( data ), 
                  std::ref( buffer ) );

   std::thread b( consumer, 
                  std::ref( data ), 
                  std::ref( buffer ) );
#endif
   a.join();
   b.join();
   return( EXIT_SUCCESS );
}
