add_subdirectory(Detours)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
add_subdirectory(freetype-2.14.1)
if(NOT TARGET Freetype::Freetype)
    add_library(Freetype::Freetype ALIAS freetype)
endif()
set(MSDFGEN_CORE_ONLY OFF CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_SVG ON CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(MSDFGEN_INSTALL OFF CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_SKIA ON CACHE BOOL "" FORCE)

add_subdirectory(msdfgen)

# chatgpt stuff
file(GLOB UD_ZIP_CANDIDATES RELATIVE "${CMAKE_SOURCE_DIR}/deps"
     "${CMAKE_SOURCE_DIR}/deps/unordered_dense*.zip")

if (UD_ZIP_CANDIDATES)
  list(GET UD_ZIP_CANDIDATES 0 UD_ZIP_NAME)
  set(UD_ZIP_PATH "${CMAKE_SOURCE_DIR}/deps/${UD_ZIP_NAME}")
else()
  message(FATAL_ERROR "unordered_dense zip not found in ${CMAKE_SOURCE_DIR}/deps. Place unordered_dense-<ver>.zip there.")
endif()

set(UD_DEST_DIR "${CMAKE_SOURCE_DIR}/deps/unordered_dense")

if (NOT EXISTS "${UD_DEST_DIR}/include")
  message(STATUS "Preparing to extract ${UD_ZIP_PATH} -> ${UD_DEST_DIR}")
  set(_ud_tmp "${CMAKE_BINARY_DIR}/_ud_extract")
  file(REMOVE_RECURSE "${_ud_tmp}")
  file(MAKE_DIRECTORY "${_ud_tmp}")

  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
    message(STATUS "Using file(ARCHIVE_EXTRACT) to unpack unordered_dense")
    file(ARCHIVE_EXTRACT INPUT "${UD_ZIP_PATH}" DESTINATION "${_ud_tmp}")
  else()
    if (WIN32)
      message(STATUS "Using PowerShell Expand-Archive to unpack unordered_dense")
      execute_process(
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -Command
          "Expand-Archive -Force -LiteralPath '${UD_ZIP_PATH}' -DestinationPath '${_ud_tmp}'"
        RESULT_VARIABLE _ud_archive_res
        OUTPUT_QUIET ERROR_VARIABLE _ud_archive_err
      )
      if (NOT _ud_archive_res EQUAL 0)
        message(FATAL_ERROR "PowerShell Expand-Archive failed: ${_ud_archive_err}")
      endif()
    else()
      find_program(UNZIP_EXE unzip)
      if (UNZIP_EXE)
        message(STATUS "Using unzip to unpack unordered_dense")
        execute_process(
          COMMAND "${UNZIP_EXE}" -o "${UD_ZIP_PATH}" -d "${_ud_tmp}"
          RESULT_VARIABLE _ud_archive_res
          OUTPUT_QUIET ERROR_VARIABLE _ud_archive_err
        )
        if (NOT _ud_archive_res EQUAL 0)
          message(FATAL_ERROR "unzip failed: ${_ud_archive_err}")
        endif()
      else()
        message(FATAL_ERROR "CMake < 3.18 and 'unzip' not found. Install unzip or use CMake >= 3.18.")
      endif()
    endif()
  endif()

  file(GLOB _ud_roots RELATIVE "${_ud_tmp}" "${_ud_tmp}/*")
  list(LENGTH _ud_roots _ud_root_count)

  if (_ud_root_count EQUAL 1)
    list(GET _ud_roots 0 _ud_single)
    set(_ud_single_path "${_ud_tmp}/${_ud_single}")
    if (EXISTS "${_ud_single_path}/include")
      file(RENAME "${_ud_single_path}" "${UD_DEST_DIR}")
    elseif (EXISTS "${_ud_tmp}/include")
      file(RENAME "${_ud_tmp}" "${UD_DEST_DIR}")
    else()
      file(MAKE_DIRECTORY "${UD_DEST_DIR}")
      file(GLOB _ud_all_items RELATIVE "${_ud_tmp}" "${_ud_tmp}/*")
      foreach(_it IN LISTS _ud_all_items)
        file(RENAME "${_ud_tmp}/${_it}" "${UD_DEST_DIR}/${_it}")
      endforeach()
    endif()
  else()
    if (EXISTS "${_ud_tmp}/include")
      file(RENAME "${_ud_tmp}" "${UD_DEST_DIR}")
    else()
      file(MAKE_DIRECTORY "${UD_DEST_DIR}")
      file(GLOB _ud_all_items RELATIVE "${_ud_tmp}" "${_ud_tmp}/*")
      foreach(_it IN LISTS _ud_all_items)
        file(RENAME "${_ud_tmp}/${_it}" "${UD_DEST_DIR}/${_it}")
      endforeach()
    endif()
  endif()

  if (EXISTS "${_ud_tmp}")
    file(REMOVE_RECURSE "${_ud_tmp}")
  endif()

  if (NOT EXISTS "${UD_DEST_DIR}/include")
    message(FATAL_ERROR "Extraction succeeded but ${UD_DEST_DIR}/include not found; check the zip contents.")
  endif()

  message(STATUS "unordered_dense is ready at ${UD_DEST_DIR}")
endif()

if (NOT TARGET ankerl::unordered_dense)
  add_library(ankerl_unordered_dense INTERFACE)
  target_include_directories(ankerl_unordered_dense INTERFACE
    "${UD_DEST_DIR}/include"
  )
  add_library(ankerl::unordered_dense ALIAS ankerl_unordered_dense)
endif()
