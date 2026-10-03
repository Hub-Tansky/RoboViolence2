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
  set_tests_properties(smoke_server PROPERTIES TIMEOUT 60)
endif()
