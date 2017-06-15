cmake_minimum_required(VERSION 3.4)
project (libhttpq-project C)
cmake_policy(SET CMP0065 NEW)
#set(CMAKE_BUILD_TYPE Release)

if (NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Set Release as default build type" FORCE)
endif()

message(STATUS "--------------------------------------")
message(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
message(STATUS "--------------------------------------")

find_library(CURL curl)

option(BUILD_SHARED_LIBS "Build library as shared" ON)

#set(_COMPILE_OPT -std=c11 -pipe -march=core-avx2 -mtune=core-avx2 -ftree-vectorize -ffast-math -flto -Wall -Wextra -Wpedantic)
set(_COMPILE_OPT -std=c11 -pipe -ftree-vectorize -ffast-math -flto -Wall -Wextra -Wpedantic)
set(_LINK_OPT "-flto -fuse-ld=gold")
set(_COMPILE_DEF -D_POSIX_C_SOURCE=199309L)

file(GLOB sources httpq.c httpq.h)
add_library(httpq ${sources})
target_compile_options(httpq PRIVATE ${_COMPILE_OPT})
set_target_properties(httpq PROPERTIES LINK_FLAGS ${_LINK_OPT})
target_compile_definitions(httpq PRIVATE ${_COMPILE_DEF})
target_link_libraries(httpq curl)

file(GLOB sample-src sample.c)
add_executable(sample ${sample-src})
#add_executable(sample EXCLUDE_FROM_ALL ${sample-src})
target_compile_options(sample PRIVATE ${_COMPILE_OPT})
set_target_properties(sample PROPERTIES LINK_FLAGS ${_LINK_OPT})
target_compile_definitions(sample PRIVATE ${_COMPILE_DEF})
target_include_directories(sample BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(sample httpq curl)
