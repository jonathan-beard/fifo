/**
 * departureratesampletype.tcc - 
 * @author: Jonathan Beard
 * @version: Thu Aug 21 12:49:40 2014
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
#ifndef _DEPARTURERATESAMPLETYPE_TCC_
#define _DEPARTURERATESAMPLETYPE_TCC_  1

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
#endif /* END _DEPARTURERATESAMPLETYPE_TCC_ */
