/**
 * monitor.hpp - 
 * @author: Jonathan Beard
 * @version: Tue Aug  5 13:22:27 2014
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
#ifndef _MONITOR_HPP_
#define _MONITOR_HPP_  1
#include <cstdint>
#include <cstring>
#include <cmath>

#include "Clock.hpp"
#include "ringbuffertypes.hpp"

#define NUMFRAMES    5
#define CONVERGENCE  .05

extern Clock *system_clock;

namespace Monitor
{
   /**
    * arrival_stats - contains to total item count and
    * frame count.
    */
   struct rate_stat
   {
      rate_stat() : items( 0 ),
                    frame_count( 0 )
      {}

      std::int64_t      items;
      std::int64_t      frame_count;
   };

   struct queue_stat
   {
      queue_stat() : items( 0 ),
                     frame_count( 0 )
      {
      }

      std::int64_t      items;
      std::int64_t      frame_count;
   };


   struct frame_resolution
   {
      frame_resolution() : curr_frame_index( 0 ),
                           curr_frame_width( 0 )
      {
         std::memset( frame_blocked, 
                      0x0, 
                      sizeof( bool ) * NUMFRAMES );
         curr_frame_width =  system_clock->getResolution(); 
      }

      static void setBlockedStatus( volatile frame_resolution &frame,
                                    Direction         dir,
                                    const bool blocked = false )
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
      static bool wasBlocked( volatile frame_resolution &frame )
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
      static bool updateResolution( volatile frame_resolution &frame,
                                    sclock_t          realized_frame_time )
      {
         const double p_diff( 
         ( realized_frame_time - frame.curr_frame_width ) /
            frame.curr_frame_width );
         fprintf( stderr, "%f\n", p_diff );
         if ( p_diff < 0 ) 
         {
            if( p_diff < ( -CONVERGENCE ) )
            {
               frame.curr_frame_width += system_clock->getResolution();
               return( false );
            }
         }
         else if ( p_diff > CONVERGENCE )
         {
            frame.curr_frame_width += system_clock->getResolution();;
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
      
      static bool acceptEntry( volatile frame_resolution &frame,
                               sclock_t                   realized_frame_time )
      {
         const float diff( realized_frame_time - frame.curr_frame_width );
         if( diff >= frame.range.lower && diff <= frame.range.upper )
         {
            return( true );
         }
         return( false );
      }

      /** might be faster with a bit vector **/
      bool     frame_blocked[ NUMFRAMES ][ 2 ];
      int      curr_frame_index;
      double   curr_frame_width;
      struct
      {
         double upper;
         double lower;
      }range;
  };

   struct QueueData 
   {
      enum Units : std::size_t { Bytes = 0, KB, MB, GB, TB, N };
      const static std::array< double, Units::N > unit_conversion;
      const static std::array< std::string, N > unit_prints;
      /**
       * QueueData - basic constructor
       * @param sample_frequency - in seconds
       * @param nbytes - number of bytes in each queue item
       */
      QueueData( size_t nbytes ) : item_unit( nbytes )
      {
      }


      static double get_arrival_rate( volatile QueueData &qd , 
                                      Units unit )
      {
         
         if( qd.arrival.items == 0 )
         {
            return( 0.0 );
         }
         return( ( (((double)qd.arrival.items ) * qd.item_unit) / 
                     (qd.resolution.curr_frame_width * 
                        (double)qd.arrival.frame_count ) ) * 
                           (double)unit_conversion[ unit ] );
      }

      static double get_departure_rate( volatile QueueData &qd, 
                                        Units unit )
      {
         if( qd.departure.items == 0 )
         {
            return( 0.0 );
         }
         return( ( (((double)qd.departure.items ) * qd.item_unit ) / 
                     (qd.resolution.curr_frame_width * 
                        (double)qd.departure.frame_count ) ) * 
                           (double)unit_conversion[ unit ] );
      }

      static double get_mean_queue_occupancy( volatile QueueData &qd ) 
      {
         if( qd.mean_occupancy.items == 0 )
         {
            return( 0 );
         }
         return( qd.mean_occupancy.items / qd.mean_occupancy.frame_count );
      }

      static double get_utilization( volatile QueueData &qd )
      {
         const auto denom( 
            QueueData::get_departure_rate( qd, Units::Bytes ) );
         if( denom == 0 )
         {
            return( 0.0 );
         }
         return( 
            QueueData::get_arrival_rate( qd, Units::Bytes ) / 
               denom );
      }

      static std::ostream& print( volatile QueueData &qd, 
                                  Units unit,
                                  std::ostream &stream,
                                  bool csv = false )
      {
         if( ! csv )
         {
            stream << "Arrival Rate: " << 
               QueueData::get_arrival_rate( qd, unit ) << " " << 
                  QueueData::unit_prints[ unit ] << "/s" << "\n";
            stream << "Departure Rate: " << 
               QueueData::get_departure_rate( qd, unit ) << " " << 
                  QueueData::unit_prints[ unit ] << "/s" << "\n";
            stream << "Mean Queue Occupancy: " << 
               QueueData::get_mean_queue_occupancy( qd ) << "\n";
            stream << "Utilization: " << 
               QueueData::get_utilization( qd );
         }
         else
         {
               stream << QueueData::get_arrival_rate( qd, unit ) << ","; 
               stream << QueueData::get_departure_rate( qd, unit ) << ","; 
               stream << QueueData::get_mean_queue_occupancy( qd ) << ",";
               stream << QueueData::get_utilization( qd );

         }
         return( stream );
      }

      rate_stat              arrival;
      rate_stat              departure;
      queue_stat             mean_occupancy;
      frame_resolution       resolution;

      const size_t           item_unit;
   };
}

const std::array< double, 
                  Monitor::QueueData::Units::N > 
                  Monitor::QueueData::unit_conversion
                      = {{ 1              /** bytes **/,
                           0.000976562    /** kilobytes **/,
                           9.53674e-7     /** megabytes **/, 
                           9.31323e-10    /** gigabytes **/,
                           9.09495e-13    /** terabytes **/ }};

const std::array< std::string, 
                  Monitor::QueueData::Units::N > 
                     Monitor::QueueData::unit_prints
                         = {{ "Bytes", "KB", "MB", "GB", "TB" }};
#endif /* END _MONITOR_HPP_ */
