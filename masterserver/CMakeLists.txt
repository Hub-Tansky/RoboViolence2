# Master server: lobby registry and ban list backed by SQLite.
add_executable(bv2master
  src/MasterClient.cpp
  src/cBV2game.cpp
  src/cMasterServer.cpp
  src/cNetManager.cpp
  src/main.cpp
)
# The master builds on babonet internals (cPacket.h, cConnection.h); see masterserver/README.md.
target_include_directories(bv2master PRIVATE src ${CMAKE_SOURCE_DIR}/engine/babonet/src)
target_link_libraries(bv2master PRIVATE babonet zeven_core ${BV2_SQLITE_TARGET} Threads::Threads)
bv2_target_defaults(bv2master)
bv2_target_legacy_strict(bv2master)
add_dependencies(bv2master bv2_seed_master_db)
if(WIN32)
  # Same manifest as bv2 and bv2dedicated (ADR 0012): Windows 10/11, UTF-8 code page
  target_sources(bv2master PRIVATE "${CMAKE_SOURCE_DIR}/packaging/windows/bv2.manifest")
endif()
