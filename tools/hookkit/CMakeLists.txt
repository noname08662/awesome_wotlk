add_subdirectory(MinHook)

set(ASMJIT_STATIC ON)
add_subdirectory(asmjit)

add_library(Hookkit INTERFACE)
target_include_directories(Hookkit INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Hookkit INTERFACE MinHook asmjit)
