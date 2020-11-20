cmake_minimum_required(VERSION 3.9)
project(test_fmod)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -march=native -mtune=native ")


find_package(Boost REQUIRED COMPONENTS timer)

include_directories(fake_incl)

add_executable(test_fmod test_fmod.cpp libm-file/e_fmod.c libm-file/e_fmod32_orig.c)
target_link_libraries(test_fmod Boost::timer)
