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
