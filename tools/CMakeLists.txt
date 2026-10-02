add_executable(bv2_seed_db_tool seed_db.cpp)
target_link_libraries(bv2_seed_db_tool PRIVATE ${BV2_SQLITE_TARGET})
set_target_properties(bv2_seed_db_tool PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(bv2_seed_db_tool PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${cfg} "${CMAKE_BINARY_DIR}/tools")
endforeach()

# Generates the runtime databases from content-seed/*.sql.
set(BV2_SEED_DIR "${CMAKE_SOURCE_DIR}/content-seed")
add_custom_command(
  OUTPUT "${BV2_RUNTIME_DIR}/bv2.db"
  COMMAND bv2_seed_db_tool "${BV2_RUNTIME_DIR}/bv2.db" "${BV2_SEED_DIR}/bv2.sql"
  DEPENDS bv2_seed_db_tool "${BV2_SEED_DIR}/bv2.sql"
  COMMENT "Generating bv2.db from content-seed/bv2.sql")
add_custom_command(
  OUTPUT "${BV2_RUNTIME_DIR}/master.db"
  COMMAND bv2_seed_db_tool "${BV2_RUNTIME_DIR}/master.db" "${BV2_SEED_DIR}/master.sql"
  DEPENDS bv2_seed_db_tool "${BV2_SEED_DIR}/master.sql"
  COMMENT "Generating master.db from content-seed/master.sql")
add_custom_command(
  OUTPUT "${BV2_RUNTIME_DIR}/web.db"
  COMMAND bv2_seed_db_tool "${BV2_RUNTIME_DIR}/web.db" "${BV2_SEED_DIR}/web.sql"
  DEPENDS bv2_seed_db_tool "${BV2_SEED_DIR}/web.sql"
  COMMENT "Generating web.db from content-seed/web.sql")
add_custom_target(bv2_seed_db DEPENDS "${BV2_RUNTIME_DIR}/bv2.db")
add_custom_target(bv2_seed_master_db DEPENDS "${BV2_RUNTIME_DIR}/master.db" "${BV2_RUNTIME_DIR}/web.db")
