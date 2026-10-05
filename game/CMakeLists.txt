# The game: client, listen server and (with CONSOLE) the dedicated server, from one codebase.

set(BV2_GAME_SOURCES
  src/AccountManager.cpp
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
  src/CSurvey.cpp
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
  src/Map.cpp
  src/MapRender.cpp
  src/MemIO.cpp
  src/Paths.cpp
  src/MenuSetup.cpp
  src/MessageDialog.cpp
  src/Minibot.cpp
  src/Player.cpp
  src/PlayerUpdate.cpp
  src/ReportGen.cpp
  src/Scene.cpp
  src/SceneNet.cpp
  src/Server.cpp
  src/ServerCTF.cpp
  src/ServerRecv.cpp
  src/ServerSnD.cpp
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
  src/Paths.cpp
  src/Minibot.cpp
  src/Player.cpp
  src/PlayerUpdate.cpp
  src/ReportGen.cpp
  src/Scene.cpp
  src/SceneNet.cpp
  src/Server.cpp
  src/ServerCTF.cpp
  src/ServerRecv.cpp
  src/ServerSnD.cpp
  src/Weapon.cpp
  src/main.cpp
  src/tinyxml.cpp
  src/tinyxmlerror.cpp
  src/tinyxmlparser.cpp
)

function(bv2_game_target target)
  target_include_directories(${target} PRIVATE src)
  target_link_libraries(${target} PRIVATE babonet dko zeven_core ${BV2_SQLITE_TARGET} Threads::Threads)
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
  add_executable(bv2dedicated ${BV2_SERVER_SOURCES})
  target_compile_definitions(bv2dedicated PRIVATE CONSOLE)
  bv2_game_target(bv2dedicated)
  target_link_libraries(bv2dedicated PRIVATE zeven_console)
  if(WIN32)
    set_target_properties(bv2dedicated PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${BV2_RUNTIME_DIR}")
  endif()
  if(TARGET bv2_placeholder_content)
    add_dependencies(bv2dedicated bv2_placeholder_content)
  endif()
endif()

if(BV2_BUILD_CLIENT)
  add_executable(bv2 ${BV2_GAME_SOURCES})
  bv2_game_target(bv2)
  target_link_libraries(bv2 PRIVATE zeven_client)
  if(WIN32)
    # Placeholder icon, generated so no icon binary is committed (the original is not redistributable)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(BV2_ICON_FILE "${CMAKE_CURRENT_BINARY_DIR}/bv2.ico")
    add_custom_command(
      OUTPUT "${BV2_ICON_FILE}"
      COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/gen-placeholder-icon.py" "${BV2_ICON_FILE}"
      DEPENDS "${CMAKE_SOURCE_DIR}/tools/gen-placeholder-icon.py"
      COMMENT "Generating placeholder icon")
    add_custom_target(bv2_icon DEPENDS "${BV2_ICON_FILE}")
    configure_file("${CMAKE_SOURCE_DIR}/packaging/windows/bv2.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/bv2.rc" @ONLY)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/bv2.rc" PROPERTIES OBJECT_DEPENDS "${BV2_ICON_FILE}")
    target_sources(bv2 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/bv2.rc" "${CMAKE_SOURCE_DIR}/packaging/windows/bv2.manifest")
    add_dependencies(bv2 bv2_icon)
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
