project( luajit )

file(GLOB VM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/extension/*.cpp")

string(REPLACE "/Ob0" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set_source_files_properties(${VM_SOURCES} PROPERTIES
    LANGUAGE CXX
    COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/extension/LuaSwapHost.h;/w;/O2")

add_library(
    ${PROJECT_NAME} STATIC
        ${VM_SOURCES}
        "extension/LuaSwapHost.h"
        "extension/LuaSwapTaint.h"
        "extension/lmemPool.h")

target_include_directories(
    ${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/tools")

target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC
        LUA_USE_CVTSD2SI
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
        Hookkit)
