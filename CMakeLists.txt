cmake_minimum_required(VERSION 3.25)

# Options come first: they select the vcpkg manifest features, which are read when project() runs the toolchain.
option(BV2_BUILD_CLIENT "Build the bv2 client" ON)
option(BV2_BUILD_SERVER "Build the bv2dedicated server" ON)
option(BV2_BUILD_TESTS "Build the tests (ctest)" ON)
option(BV2_BUILD_MASTER "Build the bv2master master server" ON)
# ADR 0002: OFF builds game/src/CCurlStub.cpp (every request ends at once with no data); ON links libcurl.
option(BV2_WITH_HTTP "Link libcurl for account and report requests" OFF)
if(NOT BV2_BUILD_CLIENT)
  set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON)
endif()
if(BV2_WITH_HTTP)
  list(APPEND VCPKG_MANIFEST_FEATURES http)
endif()

project(BaboViolent2 LANGUAGES C CXX)

# Static C++ runtime, as the x64-windows-static vcpkg ports use: players have no (debug) runtime DLLs (ADR 0012)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BV2_SANITIZE "Build with AddressSanitizer and UBSan" OFF)

find_package(Threads REQUIRED)
# GL entry points come from glad; GLU (headers and library) is still the system's (see ARCHITECTURE.md).
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

# Flags shared by every target: platform.h force-included, Windows 10 20H1 API level (ADR 0012), sanitizers.
function(bv2_target_defaults target)
  if(MSVC)
    target_compile_options(${target} PRIVATE "/FI${BV2_PLATFORM_HEADER}" /utf-8 /W4
      # Legacy code classes not worth fixing: narrowing, shadowing, unreferenced parameters, deprecated CRT.
      /wd4100 /wd4244 /wd4267 /wd4456 /wd4457 /wd4458 /wd4459 /wd4996 /wd4201 /wd4505 /wd4702 /wd4706)
    target_compile_definitions(${target} PRIVATE
      _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE NOMINMAX
      _HAS_STD_BYTE=0) # `using namespace std;` and windows.h both define `byte` otherwise
    if(BV2_SANITIZE)
      target_compile_options(${target} PRIVATE /fsanitize=address)
    endif()
  else()
    target_compile_options(${target} PRIVATE -include "${BV2_PLATFORM_HEADER}" -Wall -Wextra
      # Legacy API: string literals passed as char*, sprintf. Not worth rewriting (step 5.3).
      -Wno-deprecated-declarations -Wno-write-strings -Wno-writable-strings
      -Wno-unknown-warning-option)
    if(BV2_SANITIZE)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
  if(APPLE)
    target_compile_definitions(${target} PRIVATE GL_SILENCE_DEPRECATION)
  endif()
  if(WIN32)
    # NTDDI_WIN10_VB: the API surface of Windows 10 2004-22H2; Windows 11 APIs the SDK guards by NTDDI_VERSION fail to compile (ADR 0012)
    target_compile_definitions(${target} PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00 NTDDI_VERSION=0x0A000008)
  endif()
endfunction()

# Warnings are errors on targets that are clean. Add a target here once it builds without warnings.
function(bv2_target_strict target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /WX)
  else()
    target_compile_options(${target} PRIVATE -Werror)
  endif()
endfunction()

# Game and master code from 2012: cosmetic warning classes stay off (unused names, signed compares, initialiser order),
# everything else is an error. Real defects in these classes are fixed when found.
function(bv2_target_legacy_strict target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /wd4189 /wd4101 /wd4127 /wd4389 /wd4018 /wd4701 /wd4703 /wd4310 /wd4146)
  else()
    target_compile_options(${target} PRIVATE -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable
      -Wno-unused-private-field -Wno-unused-local-typedef -Wno-sign-compare -Wno-reorder -Wno-reorder-ctor
      -Wno-comment -Wno-dangling-else -Wno-type-limits -Wno-implicit-fallthrough
      -Wno-misleading-indentation -Wno-unused-local-typedefs -Wno-format-overflow -Wno-class-memaccess -Wno-unknown-warning-option)
  endif()
  bv2_target_strict(${target})
endfunction()

# Runtime layout: <build>/runtime/{bin, main -> content, bv2.db, master.db}
set(BV2_RUNTIME_DIR "${CMAKE_BINARY_DIR}/runtime")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BV2_RUNTIME_DIR}")
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${BV2_RUNTIME_DIR}")
endforeach()
# <runtime>/main is the game's data root: content/ entries are linked in, bv2.example.cfg is copied from config/, and placeholder assets are generated next to them (nothing is ever written into content/).
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
# Defaults only: the game keeps the user's settings in its pref directory (ADR 0007).
configure_file("${CMAKE_SOURCE_DIR}/config/bv2.example.cfg" "${BV2_DATA_DIR}/bv2.example.cfg" COPYONLY)

option(BV2_PLACEHOLDER_CONTENT "Generate placeholder maps, textures, sounds and models (the originals are not in the repo)" ON)
if(BV2_PLACEHOLDER_CONTENT)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_custom_command(
    OUTPUT "${BV2_DATA_DIR}/maps/CTF-Placeholder.bvm"
    COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/gen-placeholder-content.py" "${BV2_DATA_DIR}"
    DEPENDS "${CMAKE_SOURCE_DIR}/tools/gen-placeholder-content.py" "${CMAKE_SOURCE_DIR}/tools/placeholder-manifest.tsv"
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
if(BV2_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# Install layout (FHS; the game looks for <prefix>/share/bv2/main next to <prefix>/bin). On macOS the bundle
# holds the content in Contents/Resources instead (packaging is a later phase).
include(GNUInstallDirs)
foreach(tgt bv2 bv2dedicated bv2master)
  if(TARGET ${tgt})
    install(TARGETS ${tgt} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} BUNDLE DESTINATION .)
  endif()
endforeach()
install(DIRECTORY content/ DESTINATION ${CMAKE_INSTALL_DATADIR}/bv2/main)
install(FILES config/bv2.example.cfg DESTINATION ${CMAKE_INSTALL_DATADIR}/bv2/main)
if(UNIX AND NOT APPLE)
  install(FILES packaging/linux/roboviolence2.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()
