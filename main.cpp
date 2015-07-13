#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <unistd.h>
#include <array>
#include <sstream>
#include <fstream>
#include <cassert>
#include <cinttypes>
#include "Clock.hpp"
#include "SystemClock.tcc"
#include "procwait.hpp"
#include "ringbuffer.tcc"
#include "randomstring.tcc"
#include "signalvars.hpp"
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

#define EXP 1
//#define LIMITRATE 1
Clock *system_clock;

struct Data
{
   Data( std::int64_t send ) : send_count(  send )
   {
      gsl_rng_env_setup();
   }

   void setArrival( double mu )
   {
      r_arrival = gsl_rng_alloc( type );
      gsl_rng_set( r_arrival, 100 );
      arrival_process = mu;
   }

   void setDeparture( double mu )
   {
      r_departure = gsl_rng_alloc( type );
      gsl_rng_set( r_departure, 100 );
      departure_process = mu;
   }

   
   std::int64_t          send_count;
   double                arrival_process;
   double                departure_process;
   gsl_rng               *r_arrival;
   gsl_rng               *r_departure;
   const gsl_rng_type    *type = gsl_rng_default;
};


//#define USESharedMemory 1
#define USELOCAL 1
#define MONITOR  1
#define BUFFSIZE 64

#ifdef USESharedMemory
typedef RingBuffer< std::int64_t, 
                    Type::SharedMemory, 
                    false > TheBuffer;
#elif defined USELOCAL && defined MONITOR
typedef RingBuffer< std::int64_t          /* buffer type */,
                    Type::Heap            /* allocation type */,
                    true                  /* turn on monitoring */ >  TheBuffer;
#elif defined USELOCAL
typedef RingBuffer< std::int64_t          /* buffer type */,
                    Type::Heap            /* allocation type */,
                    false                 /* turn on monitoring */ >  TheBuffer;
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
#if LIMITRATE
#if EXP == 1
      const auto endTime( gsl_ran_exponential( data.r_departure, 
                                               data.departure_process )  + 
                           system_clock->getTime() );
#else
      const auto endTime( serviceTime + system_clock->getTime() );
#endif
   while( endTime > system_clock->getTime() );
#endif
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
#if USESharedMemory   
   return( "done" );
#elif defined MONITOR
   std::stringstream ss;
   buffer.printQueueData( ss );
   return( ss.str() );
#else
   return( "" );
#endif
}


int 
main( int argc, char **argv )
{
   if( argc != 4 )
   {
      std::cerr << "usage: " <<
         "<service time arrival> <service time departure> <items to send>\n";
      exit( EXIT_FAILURE );
   }
   
   Data data( atoi( argv[ 3 ] ) );
   data.arrival_process   = (std::stof( argv[ 1 ] ) );
   data.departure_process = (std::stof( argv[ 2 ] ) );
   
   system_clock = new SystemClock< System >( 1 );
   data.setArrival( std::stof( argv[ 1 ] ));
   data.setDeparture( std::stof( argv[ 2 ] ) );
   //RandomString< 50 > rs;
   //const std::string root( "" );
   //std::ofstream ofs( root + rs.get() + ".csv" );
   //if( ! ofs.is_open() )
   //{
   //   std::cerr << "Couldn't open ofstream!!\n";
   //   exit( EXIT_FAILURE );
   //}
   int runs( 1 );
   while( runs-- )
   {
       std::cout << test( data ) << "\n";
   }
   //ofs.close();
   if( system_clock != nullptr )
   {
      delete( system_clock );
   }
}

