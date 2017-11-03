cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(fifo)
set( version 1.2 )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

##
# Check for CXX14 or greater
##
include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( "-std=c++14" COMPILER_SUPPORTS_CXX14 )
if( COMPILER_SUPPORTS_CXX14 )
 set( CMAKE_CXX_STANDARD 14 )
else()
 message( FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a newer compiler" )
endif()


include_directories ( ${PROJECT_SOURCE_DIR} )
include_directories ( ${PROJECT_SOURCE_DIR}/include )

add_subdirectory( lib )
add_subdirectory( testsuite )

##
# enable minimal testsuite
##
set( TESTAPPS   runfifo )

enable_testing()
foreach( TEST ${TESTAPPS} )
 add_test( NAME "${TEST}_test" COMMAND ${TEST} )
endforeach( TEST ${TESTAPPS} )
