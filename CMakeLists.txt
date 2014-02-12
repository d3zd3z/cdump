# Building cdump

cmake_minimum_required(VERSION 2.6)

project(cdump)

# set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)

# Compilation options.
# First, warn a lot and consider them errors.
add_definitions(-Wall -Wextra)
add_definitions(-Werror)

# Useful for getting color, even when piped.
# add_definitions(-fcolor-diagnostics)

# Use the latest C standard.
add_definitions(-std=c++11)

######################################################################
# Make sure we have boost.
find_package(Boost 1.54 REQUIRED COMPONENTS system filesystem)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(src)

######################################################################
# And OpenSSL
find_package(OpenSSL 1.0 REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

######################################################################
# And ZLIB.
find_package(ZLIB REQUIRED)
include_directories({ZLIB_DINCLUDE_DIRS})

######################################################################
# Most of the code.
file(GLOB DumpSrc src/**.cc)
add_library(cdump ${DumpSrc})

# Test driver.
file(GLOB TestSrc test/*.cc)
add_executable(maintest ${TestSrc})
target_link_libraries(maintest gtest ${Boost_LIBRARIES})
target_link_libraries(maintest cdump)
target_link_libraries(maintest ${OPENSSL_LIBRARIES})
target_link_libraries(maintest ${ZLIB_LIBRARIES})
# target_link_libraries(maintest ${BOOST_LIBRARIES})

add_subdirectory(gtest-1.7.0)
target_include_directories(maintest PRIVATE gtest-1.7.0/include)

# Building documentation
option(BUILD_DOCUMENTATION "Build documentation")
if(BUILD_DOCUMENTATION)
  find_package(Doxygen REQUIRED)
  configure_file(Doxyfile.in
    ${PROJECT_BINARY_DIR}/Doxyfile @ONLY IMMEDIATE)
  add_custom_target(Docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
    SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
endif(BUILD_DOCUMENTATION)
