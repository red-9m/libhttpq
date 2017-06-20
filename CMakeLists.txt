cmake_minimum_required(VERSION 3.4)
project(libhttpq-project LANGUAGES C)
cmake_policy(SET CMP0065 NEW)
option(BUILD_SHARED_LIBS "Default build library as shared" ON)
if (NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Set Release as default build type" FORCE)
endif()
message(STATUS "-------------------------------------------")
message(STATUS "Build type   (CMAKE_BUILD_TYPE)  : " ${CMAKE_BUILD_TYPE})
message(STATUS "Build shared (BUILD_SHARED_LIBS) : " ${BUILD_SHARED_LIBS})
message(STATUS "-------------------------------------------")

# Sources
file(GLOB sources httpq.c httpq.h)
file(GLOB sample-src sample.c)

# Targets
add_library(httpq ${sources})
add_executable(sample ${sample-src})

# Options
# Additional: -march=core-avx2 -mtune=core-avx2
set(_COMPILE_OPT -pipe -ftree-vectorize -ffast-math -flto -Wall -Wextra -Wpedantic)
set(_LINK_OPT "-flto -fuse-ld=gold")
set(_COMPILE_DEF -D_POSIX_C_SOURCE=199309L)

# Dependency libraries
find_library(CURL curl)

# httpq
set_target_properties(httpq PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(httpq PRIVATE ${_COMPILE_OPT})
set_target_properties(httpq PROPERTIES LINK_FLAGS ${_LINK_OPT})
target_compile_definitions(httpq PRIVATE ${_COMPILE_DEF})
target_link_libraries(httpq curl)

# sample
set_target_properties(sample PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(sample PRIVATE ${_COMPILE_OPT})
set_target_properties(sample PROPERTIES LINK_FLAGS ${_LINK_OPT})
target_compile_definitions(sample PRIVATE ${_COMPILE_DEF})
target_include_directories(sample BEFORE PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(sample httpq curl)
