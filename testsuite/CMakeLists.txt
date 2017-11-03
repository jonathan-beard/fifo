set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( Threads )


set( TESTAPPS  runfifo )

include_directories( ${CMAKE_SOURCE_DIR}/include )

foreach( APP ${TESTAPPS} )
 add_executable( ${APP} "${APP}.cpp" )
 target_link_libraries( ${APP} ${CMAKE_THREAD_LIBS_INIT} fifo )
endforeach( APP ${TESTAPPS} )
