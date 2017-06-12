cmake_minimum_required(VERSION 3.4)
project (libhttpq-project C)
cmake_policy(SET CMP0065 NEW)
set(CMAKE_BUILD_TYPE Release)

file(GLOB sources httpq.c httpq.h)
file(GLOB sample-src sample.c)

add_library(httpq SHARED ${sources})
add_library(httpq-static STATIC ${sources})
#add_executable(sample EXCLUDE_FROM_ALL ${sample-src})
add_executable(sample ${sample-src})

#set(_COMPILE_OPT -std=c11 -pipe -march=core-avx2 -mtune=core-avx2 -ftree-vectorize -ffast-math -flto -Wall -Wextra -Wpedantic)
set(_COMPILE_OPT -std=c11 -pipe -ftree-vectorize -ffast-math -flto -Wall -Wextra -Wpedantic)
set(_LINK_OPT "-flto -fuse-ld=gold")
target_compile_options(httpq PRIVATE ${_COMPILE_OPT})
target_compile_options(httpq-static PRIVATE ${_COMPILE_OPT})
target_compile_options(sample PRIVATE ${_COMPILE_OPT})

set_target_properties(httpq PROPERTIES LINK_FLAGS ${_LINK_OPT})
set_target_properties(sample PROPERTIES LINK_FLAGS ${_LINK_OPT})
set_target_properties(httpq-static PROPERTIES OUTPUT_NAME httpq)

add_definitions(-D_POSIX_C_SOURCE=199309L)

find_library(CURL curl)
if (CURL)
    message(STATUS "Find curl library - done")
else()
    message(STATUS "Find curl library - Error!")
endif()

target_include_directories(sample BEFORE PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(httpq curl)
target_link_libraries(httpq-static curl)
target_link_libraries(sample httpq-static curl)
