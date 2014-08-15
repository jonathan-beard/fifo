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

namespace Monitor
{
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
      QueueData( size_t nbytes ) :
         items_arrived( 0 ),
         arrived_samples( 0 ),
         items_departed( 0 ),
         departed_samples( 0 ),
         total_occupancy( 0 ),
         item_unit( nbytes ),
         samples( 0 ),
         sample_frequency( 0 )
      {
      }

      QueueData( const QueueData &other ) : 
                           item_unit( other.item_unit ),
                           sample_frequency( other.sample_frequency )
      {
         items_arrived     = other.items_arrived;
         arrived_samples   = other.arrived_samples;
         items_departed    = other.items_departed;
         departed_samples  = other.departed_samples;
         total_occupancy   = other.total_occupancy;
         samples           = other.samples;
      }

      static double get_arrival_rate( volatile QueueData &qd , 
                                      Units unit )
      {
         
         if( qd.arrived_samples == 0 )
         {
            return( 0.0 );
         }
         return( ( (((double)qd.items_arrived ) * qd.item_unit) / 
                     (qd.sample_frequency * (double)qd.arrived_samples ) ) * 
                        (double)unit_conversion[ unit ] );
      }

      static double get_departure_rate( volatile QueueData &qd, 
                                        Units unit )
      {
         if( qd.departed_samples == 0 )
         {
            return( 0.0 );
         }
         return( ( (((double)qd.items_departed ) * qd.item_unit ) / 
                     (
                     qd.sample_frequency * (double)qd.departed_samples ) ) * 
                        (double)unit_conversion[ unit ] );
      }

      static double get_mean_queue_occupancy( volatile QueueData &qd ) 
      {
         if( qd.samples == 0 )
         {
            return( 0 );
         }
         return( qd.total_occupancy / qd.samples );
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

      std::uint64_t          items_arrived;
      std::uint64_t          arrived_samples;
      std::uint64_t          items_departed;
      std::uint64_t          departed_samples;
      std::uint64_t          total_occupancy;
      size_t                 item_unit;
      std::uint64_t          samples;
      double                 sample_frequency;
      double                 sample_time;
      double                 previous_time;
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
