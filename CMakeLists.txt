cmake_minimum_required(VERSION 3.15)

project(AwesomeWotLK LANGUAGES CXX C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_definitions(
  WIN32_LEAN_AND_MEAN
  _CRT_SECURE_NO_WARNINGS
  UNICODE
  _UNICODE
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W3 /wd4458)
  add_compile_options("$<$<CONFIG:Release>:/O2;/Gw;/GL;/DNDEBUG>")
  add_compile_options("$<$<CONFIG:Debug>:/Od;/Zi;/DDEBUG>")
  add_link_options("$<$<CONFIG:Release>:/LTCG>")
  set(CMAKE_SYSTEM_VERSION 10.0)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options("$<$<CONFIG:Release>:/O2;/Gw;/GL;/DNDEBUG>")
  add_compile_options("$<$<CONFIG:Debug>:/Od;/Zi;/DDEBUG>")
  add_link_options("$<$<CONFIG:Release>:/LTCG>")
else()
  message(FATAL_ERROR "Unsupported toolset, use MSVC or Clang for build")
endif()

add_subdirectory(deps)
add_subdirectory(src)