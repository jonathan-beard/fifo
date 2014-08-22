/**
 * resolution.cpp - 
 * @author: Jonathan Beard
 * @version: Wed Aug 20 12:53:16 2014
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
#include "resolution.hpp"
#include "Clock.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstring>

extern Clock *system_clock;

frame_resolution::frame_resolution() :
                                       curr_frame_width( 0 ),
                                       curr_frame_index( 0 )
{
   std::memset( frame_blocked, 
                0x0, 
                sizeof( bool ) * NUMFRAMES );
   curr_frame_width =  system_clock->getResolution(); 
}

void 
frame_resolution::setBlockedStatus(  frame_resolution &frame,
                                     Direction         dir,
                                     const bool blocked )
{
    frame.frame_blocked[ frame.curr_frame_index ][ (int) dir ]
      = blocked;
    frame.curr_frame_index = ( frame.curr_frame_index + 1 ) % 
                                 NUMFRAMES;
}

/**
 * wasBlocked - returns true if at any time in the 
 * previous NUMFRAMES the queue was blocked.
 * @param   frame - frame resolution reference.
 * @return  bool - true if the queue was blocked in NUMFRAMES
 */
bool 
frame_resolution::wasBlocked(  frame_resolution &frame )
{
   for( auto i( 0 ); i < NUMFRAMES; i++ )
   {
      if( frame.frame_blocked[ i ][ 0 ] || frame.frame_blocked[ i ][ 1 ] )
      {
         return( true );
      }
   }
   return( false );
}

/**
 * updateResolution - function gets called at each iteration 
 * of the monitor loop until convergence is reached at which 
 * point this function returns true.
 * @param   frame - frame_resolution struct
 * @param   realized_frame_time - actual time to go through loop
 * @return  bool
 */
bool 
frame_resolution::updateResolution(  frame_resolution &frame,
                                     sclock_t          prev_time )
{
   
   const auto realized_frame_time( system_clock->getTime() - prev_time );
   const double p_diff( 
   ( realized_frame_time - frame.curr_frame_width ) /
      frame.curr_frame_width );
   fprintf( stderr, "%.20f,%.20f,%.20f\n", p_diff, 
                                           frame.curr_frame_width,
                                           realized_frame_time );
   if ( p_diff < 0 ) 
   {
      if( p_diff < ( -CONVERGENCE ) )
      {
         //frame.curr_frame_width += system_clock->getResolution();
         frame.curr_frame_width *= 2;
         return( false );
      }
   }
   else if ( p_diff > CONVERGENCE )
   {
      //frame.curr_frame_width += system_clock->getResolution();
      frame.curr_frame_width *= 2;
      return( false );
   }
   //else calc range
   const double upperPercent( 1.25 );
   const double lowerPercent( .75   );
   /** note: frame.curr_frame_width always > 0 **/
   frame.range.upper = frame.curr_frame_width * upperPercent;
   frame.range.lower = frame.curr_frame_width * lowerPercent;
   return( true );
}

bool 
frame_resolution::acceptEntry(  frame_resolution   &frame,
                                sclock_t            realized_frame_time )
{
   const float diff( realized_frame_time - frame.curr_frame_width );
   if( diff >= frame.range.lower && diff <= frame.range.upper )
   {
      return( true );
   }
   return( false );
}

void 
frame_resolution::waitForInterval(  frame_resolution &frame )
{
   const sclock_t stop_time( system_clock->getTime() + frame.curr_frame_width );
   while( system_clock->getTime() < stop_time ) 
   {
#if __x86_64            
      __asm__ ("\
         pause"
         :
         :
         : );
#endif               
   }
   return;
}

sclock_t
frame_resolution::getFrameWidth()
{
   return( curr_frame_width );
}
