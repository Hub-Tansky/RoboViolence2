# The game: client, listen server and (with CONSOLE) the dedicated server, from one codebase.

set(BV2_GAME_SOURCES
  src/AccountManager.cpp
  src/AdminApi.cpp
  src/Autotest.cpp
  src/Button.cpp
  src/CAStar.cpp
  src/CAStar_FindPath.cpp
  src/CAccount.cpp
  src/CBrowser.cpp
  src/CClans.cpp
  src/CControl.cpp
  src/CCredit.cpp
  src/CUrlData.cpp
  src/CEditor.cpp
  src/CFriends.cpp
  src/CHost.cpp
  src/CLava.cpp
  src/CListener.cpp
  src/CLobby.cpp
  src/CMainTab.cpp
  src/CMaster.cpp
  src/CMaterial.cpp
  src/CMenuManager.cpp
  src/CMesh.cpp
  src/CMeshBuilder.cpp
  src/CNews.cpp
  src/COption.cpp
  src/CPathNode.cpp
  src/CPing.cpp
  src/CPlayers.cpp
  src/CProfile.cpp
  src/CRain.cpp
  src/CRegisterClan.cpp
  src/CSnow.cpp
  src/CStats.cpp
  src/CStatus.cpp
  src/CUserLogin.cpp
  src/CVertexBuffer.cpp
  src/CWeather.cpp
  src/Choice.cpp
  src/Client.cpp
  src/ClientRecv.cpp
  src/ClientRender.cpp
  src/ConfirmPass.cpp
  src/Console.cpp
  src/Control.cpp
  src/ControlListener.cpp
  src/Dialog.cpp
  src/Editor.cpp
  src/EditorDialogs.cpp
  src/EditorTools.cpp
  src/FileIO.cpp
  src/Game.cpp
  src/GameProjectile.cpp
  src/GameRender.cpp
  src/GameShowStats.cpp
  src/GameSpawn.cpp
  src/GameVar.cpp
  src/Helper.cpp
  src/IntroScreen.cpp
  src/Key.cpp
  src/KeyManager.cpp
  src/Label.cpp
  src/Lang.cpp
  src/Map.cpp
  src/MapRender.cpp
  src/MemIO.cpp
  src/PacketCheck.cpp
  src/DataDirPicker.cpp
  src/Paths.cpp
  src/MenuSetup.cpp
  src/MessageDialog.cpp
  src/Minibot.cpp
  src/Player.cpp
  src/PlayerUpdate.cpp
  src/ReportGen.cpp
  src/Retention.cpp
  src/Scene.cpp
  src/SceneNet.cpp
  src/Server.cpp
  src/ServerCTF.cpp
  src/ServerRecv.cpp
  src/ServerSnD.cpp
  src/ServerValidator.cpp
  src/Weapon.cpp
  src/Write.cpp
  src/Writting.cpp
  src/main.cpp
  src/screengrab.cpp
  src/tinyxml.cpp
  src/tinyxmlerror.cpp
  src/tinyxmlparser.cpp
)

# Files that are empty under CONSOLE (menus, editor, client, rendering) are left out.
set(BV2_SERVER_SOURCES
  src/AccountManager.cpp
  src/AdminApi.cpp
  src/CAStar.cpp
  src/CAStar_FindPath.cpp
  src/CUrlData.cpp
  src/CMaster.cpp
  src/CPathNode.cpp
  src/CPing.cpp
  src/CStatus.cpp
  src/Console.cpp
  src/FileIO.cpp
  src/Game.cpp
  src/GameProjectile.cpp
  src/GameSpawn.cpp
  src/GameVar.cpp
  src/Helper.cpp
  src/Map.cpp
  src/MapRender.cpp
  src/MemIO.cpp
  src/PacketCheck.cpp
  src/Paths.cpp
  src/Minibot.cpp
  src/Player.cpp
  src/PlayerUpdate.cpp
  src/ReportGen.cpp
  src/Retention.cpp
  src/Scene.cpp
  src/SceneNet.cpp
  src/Server.cpp
  src/ServerCTF.cpp
  src/ServerRecv.cpp
  src/ServerSnD.cpp
  src/ServerValidator.cpp
  src/Weapon.cpp
  src/main.cpp
  src/tinyxml.cpp
  src/tinyxmlerror.cpp
  src/tinyxmlparser.cpp
)

# visibility: PRIVATE for executables, PUBLIC for bv2server_lib, whose dependents (bv2dedicated, tests) need the same
# includes and libraries
function(bv2_game_target target visibility)
  target_include_directories(${target} ${visibility} src)
  target_link_libraries(${target} ${visibility} babonet credentials dko zeven_core ${BV2_SQLITE_TARGET} unofficial-sodium::sodium Threads::Threads)
  if(BV2_WITH_HTTP)
    target_sources(${target} PRIVATE src/CCurl.cpp)
    target_compile_definitions(${target} PRIVATE BV2_WITH_HTTP)
    target_link_libraries(${target} PRIVATE CURL::libcurl)
  else()
    target_sources(${target} PRIVATE src/CCurlStub.cpp)
  endif()
  bv2_target_defaults(${target})
  bv2_target_legacy_strict(${target})
endfunction()

if(BV2_BUILD_SERVER)
  # The dedicated server minus main.cpp. Tests link it too (tests/harness): its fake bb_* object file wins over
  # babonet's archive, so baboNet.cpp is never pulled in and the server talks to a fake network.
  set(BV2_SERVER_LIB_SOURCES ${BV2_SERVER_SOURCES})
  list(REMOVE_ITEM BV2_SERVER_LIB_SOURCES src/main.cpp)
  add_library(bv2server_lib STATIC ${BV2_SERVER_LIB_SOURCES})
  target_compile_definitions(bv2server_lib PUBLIC CONSOLE)
  bv2_game_target(bv2server_lib PUBLIC)
  target_link_libraries(bv2server_lib PUBLIC zeven_console)

  add_executable(bv2dedicated src/main.cpp)
  target_link_libraries(bv2dedicated PRIVATE bv2server_lib)
  bv2_target_defaults(bv2dedicated)
  bv2_target_legacy_strict(bv2dedicated)
  if(WIN32)
    set_target_properties(bv2dedicated PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${BV2_RUNTIME_DIR}")
    # UTF-8 code page for the narrow paths in Paths.cpp (ADR 0012); the DPI entries don't affect console programs
    target_sources(bv2dedicated PRIVATE "${CMAKE_SOURCE_DIR}/packaging/windows/bv2.manifest")
  endif()
  if(TARGET bv2_placeholder_content)
    add_dependencies(bv2dedicated bv2_placeholder_content)
  endif()
endif()

if(BV2_BUILD_CLIENT AND UNIX AND NOT APPLE)
  # AppIcon.h: g_appIconPng, the bytes of the 128 px hicolor icon, for the Linux window icon (ADR 0030).
  # Also linked by tests/test_window_icon.
  set(BV2_ICON_PNG "${CMAKE_SOURCE_DIR}/packaging/icons/hicolor/128x128/apps/roboviolence2.png")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${BV2_ICON_PNG}")
  file(READ "${BV2_ICON_PNG}" icon_hex HEX)
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," icon_bytes "${icon_hex}")
  file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/AppIcon.h"
    CONTENT "// Generated by game/CMakeLists.txt from packaging/icons/hicolor/128x128/apps/roboviolence2.png\n#pragma once\nstatic const unsigned char g_appIconPng[] = {${icon_bytes}};\n")
  add_library(bv2_app_icon INTERFACE)
  target_include_directories(bv2_app_icon INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_compile_definitions(bv2_app_icon INTERFACE BV2_APP_ICON_PNG)
endif()

if(BV2_BUILD_CLIENT)
  add_executable(bv2 ${BV2_GAME_SOURCES})
  bv2_game_target(bv2 PRIVATE)
  target_link_libraries(bv2 PRIVATE zeven_client)
  # Application icon (ADR 0030): committed art from tools/gen-app-icon.py
  set(BV2_ICONS "${CMAKE_SOURCE_DIR}/packaging/icons")
  if(WIN32)
    configure_file("${CMAKE_SOURCE_DIR}/packaging/windows/bv2.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/bv2.rc" @ONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/bv2.rc" PROPERTIES OBJECT_DEPENDS "${BV2_ICONS}/roboviolence2.ico")
    target_sources(bv2 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/bv2.rc" "${CMAKE_SOURCE_DIR}/packaging/windows/bv2.manifest")
  elseif(APPLE)
    target_sources(bv2 PRIVATE "${BV2_ICONS}/roboviolence2.icns")
    set_source_files_properties("${BV2_ICONS}/roboviolence2.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  else()
    # Linux: no bundle or resource section, so the window icon is compiled in (bv2_app_icon: AppIcon.h)
    target_link_libraries(bv2 PRIVATE bv2_app_icon)
  endif()
  if(WIN32)
    # Visual Studio generator (win-x64-vs): debug from the runtime dir, where main/ lives.
    set_target_properties(bv2 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${BV2_RUNTIME_DIR}")
    set_target_properties(bv2 PROPERTIES WIN32_EXECUTABLE ON)
    # GUI subsystem (no console window) but the portable main() stays the entry point.
    if(MSVC)
      target_link_options(bv2 PRIVATE /ENTRY:mainCRTStartup)
    endif()
  elseif(APPLE)
    set_target_properties(bv2 PROPERTIES MACOSX_BUNDLE ON
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/packaging/macos/Info.plist.in")
    # Development layout: the bundle finds main/ in Contents/Resources (the first place the game looks)
    add_custom_command(TARGET bv2 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:bv2>/Resources"
      COMMAND ${CMAKE_COMMAND} -E create_symlink "${BV2_DATA_DIR}" "$<TARGET_BUNDLE_CONTENT_DIR:bv2>/Resources/main"
      COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:bv2>" || true
      VERBATIM)
  endif()
  if(TARGET bv2_placeholder_content)
    add_dependencies(bv2 bv2_placeholder_content)
  endif()
endif()
