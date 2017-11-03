#include <cstdlib>
#include <iostream>
#include <thread>
#include <cstdint>
#include <cassert>
#include <cinttypes>
#include <vector>
#include "ringbuffer.tcc"
#include "signalvars.hpp"



std::vector< std::int64_t > test_data;

struct Data
{
   Data( std::int64_t send ) : send_count(  send )
   {
   }

   void setArrival( double mu )
   {
      arrival_process = mu;
   }

   void setDeparture( double mu )
   {
      departure_process = mu;
   }

   
   std::int64_t          send_count;
   double                arrival_process;
   double                departure_process;
};


//#define USESharedMemory 1
#define USELOCAL 1
#define BUFFSIZE 64

#ifdef USESharedMemory
typedef RingBuffer< std::int64_t, 
                    Type::SharedMemory 
                    > TheBuffer;
#elif defined USELOCAL
typedef RingBuffer< std::int64_t          /* buffer type */,
                    Type::Heap            /* allocation type */
                    >  TheBuffer;
#endif




void
producer( Data &data, TheBuffer &buffer )
{
   std::int64_t current_count( 0 );
   while( current_count++ < data.send_count )
   {
      auto &ref( buffer.allocate< std::int64_t >() );
      ref = current_count;
      buffer.push( /* current_count, */ 
         (current_count == data.send_count ? 
          RBSignal::RBEOF : RBSignal::NONE ) );
#if LIMITRATE
#if EXP == 1
      const auto endTime( gsl_ran_exponential( data.r_arrival, 
                                               data.arrival_process )  + 
                           system_clock->getTime() );
#else
      const auto endTime( serviceTime + system_clock->getTime() );
#endif
      while( endTime > system_clock->getTime() );
#endif
   }
   return;
}

void 
consumer( Data &data, TheBuffer &buffer )
{
   std::int64_t   current_count( 0 );
   RBSignal signal( RBSignal::NONE );
   while( signal != RBSignal::RBEOF )
   {
      buffer.pop( current_count, &signal );
      test_data.emplace_back( current_count );
      //if this exits at the end, all should be good **/
   }
   assert( current_count == data.send_count );
   return;
}

std::string test( Data &data )
{
#ifdef USESharedMemory
   char shmkey[ 256 ];
   SHM::GenKey( shmkey, 256 );
   std::string key( shmkey );
   ProcWait *proc_wait( new ProcWait( 1 ) ); 
   const pid_t child( fork() );
   switch( child )
   {
      case( 0 /* CHILD */ ):
      {
         TheBuffer buffer_b( BUFFSIZE,
                             key, 
                             Direction::Consumer, 
                             false);
         /** call consumer function directly **/
         consumer( buffer_b );
         exit( EXIT_SUCCESS );
      }
      break;
      case( -1 /* failed to fork */ ):
      {
         std::cerr << "Failed to fork, exiting!!\n";
         exit( EXIT_FAILURE );
      }
      break;
      default: /* parent */
      {
         proc_wait->AddProcess( child );
         TheBuffer buffer_a( BUFFSIZE,
                             key, 
                             Direction::Producer, 
                             false);
         /** call producer directly **/
         producer( data, buffer_a );
      }
   }
  
   /** parent waits for child **/
   proc_wait->WaitForChildren();

   delete( proc_wait );
   
#elif defined USELOCAL
   TheBuffer buffer( BUFFSIZE );
   std::thread a( producer, 
                  std::ref( data ), 
                  std::ref( buffer ) );

   std::thread b( consumer, 
                  std::ref( data ),
                  std::ref( buffer ) );
   a.join();
   b.join();
#endif
   return( "done" );
}


int 
main( int argc, char **argv )
{
   
   Data data( 1000 );
   data.arrival_process   = ( 10 );
   data.departure_process = ( 10 );
   
   data.setArrival( 10 );
   data.setDeparture( 10 );
   
   
   std::cout << test( data ) << "\n";
   for( auto i( 1 ); i < test_data.size(); i++ )
   {
           assert( test_data[ i-1 ] = i );
   }
   exit( 0 );
}

