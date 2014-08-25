
               const Blocked read_copy ( buffer.read_stats );
               const Blocked write_copy( buffer.write_stats );
               buffer.read_stats.all   = 0;
               buffer.write_stats.all  = 0;
               if( ! arrival_started )
               {
                  if( write_copy.count != 0 )
                  {
                     arrival_started = true;
                     std::this_thread::yield();
                     continue;
                  }
               }
               /**
                * if we're not blocked, and the server has actually started
                * and the end of data signal has not been received then 
                * record the throughput within this frame
                */
               if( write_copy.blocked == 0 && 
                     arrival_started  && ! buffer.write_finished ) 
               {
                  Monitor::frame_resolution::setBlockedStatus( 
                                                      data.resolution,
                                                      Direction::Producer,
                                                      false );
                  if( converged 
                     && Monitor::frame_resolution::acceptEntry( data.resolution,
                                                     system_clock->getTime() - prev_time ) )
                  {
                     data.arrival.items         += write_copy.count;
                     data.arrival.frame_count   += 1;
                  }
                  else
                  {
                     data.arrival.items         += 0;
                     data.arrival.frame_count   += 0;
                  }
               }
               else
               {
                  Monitor::frame_resolution::setBlockedStatus( data.resolution,
                                                      Direction::Producer,
                                                      true );
               }




               /**
                * if we're not blocked, and the server has actually started
                * and the end of data signal has not been received then 
                * record the throughput within this frame
                */
               if( read_copy.blocked == 0 )
               {
                  Monitor::frame_resolution::setBlockedStatus( data.resolution,
                                                      Direction::Consumer,
                                                      false );
                  if( converged 
                        && Monitor::frame_resolution::acceptEntry( data.resolution,
                                                  system_clock->getTime() - prev_time ) )
                  {
                     data.departure.items       += read_copy.count;
                     data.departure.frame_count += 1;
                  }
                  else
                  {
                     data.departure.items       += 0;
                     data.departure.frame_count += 0;
                  }
               }
               else
               {
                  Monitor::frame_resolution::setBlockedStatus( 
                                                      data.resolution,
                                                      Direction::Consumer,
                                                      true );
               }
