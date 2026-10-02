cmake_minimum_required(VERSION 3.25)

project(BaboViolent2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BV2_BUILD_CLIENT "Build the bv2 client (needs the Step 3 platform layer)" ON)
option(BV2_BUILD_SERVER "Build the bv2dedicated server" ON)
option(BV2_BUILD_MASTER "Build the bv2master master server" ON)
# ADR 0002: Step 3 adds the CCurl stub and flips this to OFF. Until then CCurl.cpp needs libcurl.
option(BV2_WITH_HTTP "Link libcurl for account and report requests" ON)
option(BV2_SANITIZE "Build with AddressSanitizer and UBSan" OFF)

find_package(Threads REQUIRED)
# The headless server still links dkt and dko, which call GL/GLU for texture upload (see ARCHITECTURE.md).
find_package(OpenGL REQUIRED COMPONENTS OpenGL OPTIONAL_COMPONENTS GLU)
find_package(SQLite3 REQUIRED)
# CMake 4.x renames the imported target; keep one name for both.
if(TARGET SQLite3::SQLite3)
  set(BV2_SQLITE_TARGET SQLite3::SQLite3)
else()
  set(BV2_SQLITE_TARGET SQLite::SQLite3)
endif()
if(BV2_WITH_HTTP)
  find_package(CURL REQUIRED)
endif()

set(BV2_PLATFORM_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/engine/zeven/include/platform.h")

# Flags shared by every target: platform.h force-included, Windows 10/11 API level, sanitizers.
function(bv2_target_defaults target)
  if(MSVC)
    target_compile_options(${target} PRIVATE "/FI${BV2_PLATFORM_HEADER}" /utf-8)
    target_compile_definitions(${target} PRIVATE
      _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE NOMINMAX)
    if(BV2_SANITIZE)
      target_compile_options(${target} PRIVATE /fsanitize=address)
    endif()
  else()
    target_compile_options(${target} PRIVATE -include "${BV2_PLATFORM_HEADER}")
    if(BV2_SANITIZE)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
  if(APPLE)
    target_compile_definitions(${target} PRIVATE GL_SILENCE_DEPRECATION)
  endif()
  if(WIN32)
    target_compile_definitions(${target} PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00)
  endif()
endfunction()

# Runtime layout: <build>/runtime/{bin, main -> content, bv2.db, master.db}
set(BV2_RUNTIME_DIR "${CMAKE_BINARY_DIR}/runtime")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BV2_RUNTIME_DIR}")
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${BV2_RUNTIME_DIR}")
endforeach()
# <runtime>/main is the game's data root: content/ entries are linked in, bv2.cfg is copied from the
# example, and placeholder assets are generated next to them (nothing is ever written into content/).
set(BV2_DATA_DIR "${BV2_RUNTIME_DIR}/main")
file(MAKE_DIRECTORY "${BV2_DATA_DIR}")
function(bv2_link_dir src dst)
  if(EXISTS "${dst}")
    return()
  endif()
  file(CREATE_LINK "${src}" "${dst}" SYMBOLIC RESULT result)
  if(NOT result STREQUAL "0" AND WIN32)
    file(TO_NATIVE_PATH "${dst}" dst_native)
    file(TO_NATIVE_PATH "${src}" src_native)
    execute_process(COMMAND cmd /c mklink /J "${dst_native}" "${src_native}" OUTPUT_QUIET RESULT_VARIABLE result)
  endif()
  if(NOT result STREQUAL "0")
    message(WARNING "Could not link ${dst} to ${src}; copy it by hand.")
  endif()
endfunction()
foreach(entry languages LaunchScript)
  bv2_link_dir("${CMAKE_SOURCE_DIR}/content/${entry}" "${BV2_DATA_DIR}/${entry}")
endforeach()
configure_file("${CMAKE_SOURCE_DIR}/config/bv2.example.cfg" "${BV2_DATA_DIR}/bv2.cfg" COPYONLY)

option(BV2_PLACEHOLDER_CONTENT "Generate placeholder maps, textures, sounds and models (the originals are not in the repo)" ON)
if(BV2_PLACEHOLDER_CONTENT)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_custom_command(
    OUTPUT "${BV2_DATA_DIR}/maps/CTF-Placeholder.bvm"
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/gen-placeholder-content.py" "${BV2_DATA_DIR}"
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/gen-placeholder-content.py" "${CMAKE_SOURCE_DIR}/docs/assets/ASSET-INVENTORY.md"
    COMMENT "Generating placeholder game data")
  add_custom_target(bv2_placeholder_content DEPENDS "${BV2_DATA_DIR}/maps/CTF-Placeholder.bvm")
endif()

add_subdirectory(engine/zeven)
add_subdirectory(engine/babonet)
add_subdirectory(engine/dko)
add_subdirectory(tools)
if(BV2_BUILD_CLIENT OR BV2_BUILD_SERVER)
  add_subdirectory(game)
endif()
if(BV2_BUILD_MASTER)
  add_subdirectory(masterserver)
endif()
