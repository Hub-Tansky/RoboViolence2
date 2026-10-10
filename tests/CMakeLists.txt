add_executable(test_netpacket test_netpacket.cpp)
target_include_directories(test_netpacket PRIVATE ${CMAKE_SOURCE_DIR}/game/src ${CMAKE_SOURCE_DIR}/engine/babonet/include)
bv2_target_defaults(test_netpacket)
add_test(NAME netpacket COMMAND test_netpacket)

add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE zeven_core)
bv2_target_defaults(test_config)
set(BV2_TEST_SCRATCH "${CMAKE_BINARY_DIR}/test-scratch")
file(MAKE_DIRECTORY "${BV2_TEST_SCRATCH}")
add_test(NAME config COMMAND test_config "${BV2_TEST_SCRATCH}")

if(BV2_BUILD_SERVER)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME smoke_server
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/smoke_server.py $<TARGET_FILE:bv2dedicated> "${BV2_TEST_SCRATCH}/smoke-pref")
  set_tests_properties(smoke_server PROPERTIES TIMEOUT 60 RESOURCE_LOCK bv2_server_port)
  if(WIN32)  # non-ASCII install path needs the UTF-8 code page manifest (ADR 0012)
    add_test(NAME smoke_server_utf8_path
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/smoke_server.py $<TARGET_FILE:bv2dedicated> "${BV2_TEST_SCRATCH}/utf8-pref"
        --install-under "${BV2_TEST_SCRATCH}/utf8-path")
    set_tests_properties(smoke_server_utf8_path PROPERTIES TIMEOUT 60 RESOURCE_LOCK bv2_server_port)
  endif()
endif()

if(TARGET bv2server_lib)
  # In-process server on a fake babonet (tests/harness): an OBJECT library, so its bb_* win over babonet's archive.
  add_library(bv2_server_harness OBJECT harness/fake_babonet.cpp harness/server_harness.cpp)
  target_include_directories(bv2_server_harness PUBLIC harness)
  target_link_libraries(bv2_server_harness PUBLIC bv2server_lib)
  bv2_target_defaults(bv2_server_harness)

  add_executable(test_server_harness test_server_harness.cpp)
  target_link_libraries(test_server_harness PRIVATE bv2_server_harness)
  bv2_target_defaults(test_server_harness)
  if(TARGET bv2_placeholder_content)
    add_dependencies(bv2_server_harness bv2_placeholder_content)
  endif()
  add_test(NAME server_harness COMMAND test_server_harness)
  set_tests_properties(server_harness PROPERTIES ENVIRONMENT "BV2_PREF_DIR=${BV2_TEST_SCRATCH}/harness-pref")

  # Corpus (tests/corpus/<type>/*.bin): gen_corpus writes valid.bin per type; replay runs every file except crash-*.
  add_executable(gen_corpus gen_corpus.cpp)
  target_include_directories(gen_corpus PRIVATE ${CMAKE_SOURCE_DIR}/game/src ${CMAKE_SOURCE_DIR}/engine/babonet/include)
  bv2_target_defaults(gen_corpus)
  add_executable(test_replay test_replay.cpp)
  target_link_libraries(test_replay PRIVATE bv2_server_harness)
  bv2_target_defaults(test_replay)
  set(BV2_CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/corpus")
  add_test(NAME replay COMMAND test_replay "${BV2_CORPUS}")
  set_tests_properties(replay PROPERTIES ENVIRONMENT "BV2_PREF_DIR=${BV2_TEST_SCRATCH}/replay-pref")
  # Fuzzer (not a ctest; CI job `fuzz` runs it per type): fuzz_server <corpus> <type> <seconds> <seed> <out>
  add_executable(fuzz_server fuzz_server.cpp)
  target_link_libraries(fuzz_server PRIVATE bv2_server_harness)
  bv2_target_defaults(fuzz_server)
  # Known defects (crash-*.bin, KEY_QUESTIONS.md): passes only while ASan reports the defect; its fix step deletes the file.
  if(BV2_SANITIZE)
    file(GLOB BV2_CRASH_INPUTS CONFIGURE_DEPENDS "${BV2_CORPUS}/*/crash-*.bin")
    foreach(input ${BV2_CRASH_INPUTS})
      get_filename_component(dir "${input}" DIRECTORY)
      get_filename_component(type "${dir}" NAME)
      get_filename_component(name "${input}" NAME_WE)
      add_test(NAME replay_${type}_${name} COMMAND test_replay "${BV2_CORPUS}" "${input}")
      set_tests_properties(replay_${type}_${name} PROPERTIES PASS_REGULAR_EXPRESSION "delivering crash input.*ERROR: AddressSanitizer"
        ENVIRONMENT "BV2_PREF_DIR=${BV2_TEST_SCRATCH}/replay-pref;ASAN_OPTIONS=abort_on_error=0:detect_leaks=0")
    endforeach()
  endif()
endif()

if(TARGET bv2master_lib)
  add_executable(test_master test_master.cpp)
  target_link_libraries(test_master PRIVATE bv2master_lib)
  bv2_target_defaults(test_master)
  add_dependencies(test_master bv2_seed_master_db)
  # Not on Windows (owner, 2026-10-10): R28, an intermittent babonet crash when a connection closes; step 5 re-enables it.
  if(NOT WIN32)
    add_test(NAME master COMMAND test_master "${BV2_RUNTIME_DIR}" "${BV2_TEST_SCRATCH}/master")
    set_tests_properties(master PROPERTIES TIMEOUT 60 RESOURCE_LOCK bv2_master_port)
  endif()
endif()

add_executable(test_fileio test_fileio.cpp ${CMAKE_SOURCE_DIR}/game/src/FileIO.cpp)
target_include_directories(test_fileio PRIVATE ${CMAKE_SOURCE_DIR}/game/src)
target_link_libraries(test_fileio PRIVATE zeven_core)
bv2_target_defaults(test_fileio)
add_test(NAME fileio COMMAND test_fileio "${BV2_TEST_SCRATCH}")

if(WIN32 AND TARGET bv2 AND TARGET bv2dedicated AND TARGET bv2master)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME windows_no_crt_dlls
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_windows_deps.py
      $<TARGET_FILE:bv2> $<TARGET_FILE:bv2dedicated> $<TARGET_FILE:bv2master>)
endif()
