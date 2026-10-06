cmake_minimum_required(VERSION 3.24)
project(blpcodec LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.28.0")
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

option(BLPCODEC_BUILD_EXE "Build the standalone CLI tool" ${PROJECT_IS_TOP_LEVEL})

add_library(blpcodec "${CMAKE_CURRENT_SOURCE_DIR}/src/blpcodec.cpp")
add_library(blpcodec::blpcodec ALIAS blpcodec)

target_include_directories(blpcodec
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/deps"
)

# MSVC is also set for clang-cl, whose -Wall means -Weverything
if(MSVC)
    target_compile_options(blpcodec PRIVATE /W4 /permissive- /Zc:__cplusplus /EHsc)
else()
    target_compile_options(blpcodec PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set_property(TARGET blpcodec PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(BLPCODEC_BUILD_EXE)
    add_executable(blpcodec_cli "${CMAKE_CURRENT_SOURCE_DIR}/apps/main.cpp")
    target_link_libraries(blpcodec_cli PRIVATE blpcodec)
    target_include_directories(blpcodec_cli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps")
    set_target_properties(blpcodec_cli PROPERTIES 
        OUTPUT_NAME "blpcodec" 
        PROJECT_LABEL "blpcodec CLI"
    )
    if(ipo_supported)
        set_property(TARGET blpcodec_cli PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()