cmake_minimum_required(VERSION 4.2)

project(
    libhttpq
    VERSION 1.0.0
    DESCRIPTION "Small synchronous HTTP/HTTPS POST wrapper around libcurl"
    LANGUAGES C
)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "Build libhttpq as a shared library" ON)
option(LIBHTTPQ_BUILD_SAMPLE "Build the Telegram sample program" ${PROJECT_IS_TOP_LEVEL})
option(LIBHTTPQ_ENABLE_IPO "Enable interprocedural optimization when supported" ${PROJECT_IS_TOP_LEVEL})

find_package(CURL 8.11.1 REQUIRED)
find_package(Threads REQUIRED)

function(libhttpq_enable_warnings target_name)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(${target_name} PRIVATE /W4)
    elseif(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang|IntelLLVM)$")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

set(libhttpq_ipo_supported OFF)
if(LIBHTTPQ_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(
        RESULT libhttpq_ipo_supported
        OUTPUT libhttpq_ipo_error
        LANGUAGES C
    )
    if(NOT libhttpq_ipo_supported)
        message(WARNING "IPO is unavailable and will be disabled: ${libhttpq_ipo_error}")
    endif()
endif()

add_library(httpq httpq.c)
add_library(libhttpq::httpq ALIAS httpq)

target_sources(
    httpq
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES httpq.h
)
target_compile_definitions(httpq PRIVATE _POSIX_C_SOURCE=199309L)
target_link_libraries(httpq PRIVATE CURL::libcurl Threads::Threads)
target_include_directories(
    httpq
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
set_target_properties(
    httpq
    PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
)
libhttpq_enable_warnings(httpq)
if(libhttpq_ipo_supported AND BUILD_SHARED_LIBS)
    set_property(TARGET httpq PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
endif()

if(LIBHTTPQ_BUILD_SAMPLE)
    add_executable(httpq-sample sample.c)
    set_target_properties(
        httpq-sample
        PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF
    )
    target_link_libraries(httpq-sample PRIVATE libhttpq::httpq)
    libhttpq_enable_warnings(httpq-sample)
    if(libhttpq_ipo_supported)
        set_property(TARGET httpq-sample PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

install(
    TARGETS httpq
    EXPORT libhttpqTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

set(libhttpq_cmake_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/libhttpq")
configure_package_config_file(
    cmake/libhttpqConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/libhttpqConfig.cmake"
    INSTALL_DESTINATION "${libhttpq_cmake_install_dir}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libhttpqConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

install(
    EXPORT libhttpqTargets
    FILE libhttpqTargets.cmake
    NAMESPACE libhttpq::
    DESTINATION "${libhttpq_cmake_install_dir}"
)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/libhttpqConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/libhttpqConfigVersion.cmake"
    DESTINATION "${libhttpq_cmake_install_dir}"
)
