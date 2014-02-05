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

# Use the latest C standard.
add_definitions(-std=c++11)

# Make sure we have boost.
find_package(Boost 1.54 REQUIRED COMPONENTS system filesystem)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(src)

# And OpenSSL
find_package(OpenSSL 1.0 REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# Most of the code.
file(GLOB DumpSrc src/**.cc)
add_library(cdump ${DumpSrc})

# Test driver.
file(GLOB TestSrc test/*.cc)
add_executable(maintest ${TestSrc})
target_link_libraries(maintest gtest ${Boost_LIBRARIES})
target_link_libraries(maintest cdump)
target_link_libraries(maintest gtest ${OPENSSL_LIBRARIES})
# target_link_libraries(maintest ${BOOST_LIBRARIES})

add_subdirectory(gtest-1.7.0)
target_include_directories(maintest PRIVATE gtest-1.7.0/include)
