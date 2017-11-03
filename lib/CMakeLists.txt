set( CMAKE_INCLUDE_CURRENT_DIR ON )

add_library( fifo fifo.cpp pointer.cpp )
install( TARGETS fifo
         ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
