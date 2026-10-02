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
  src/CCurl.cpp
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
  src/CCurl.cpp
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
    target_link_libraries(${target} PRIVATE CURL::libcurl)
  endif()
  bv2_target_defaults(${target})
endfunction()

if(BV2_BUILD_SERVER)
  add_executable(bv2dedicated ${BV2_SERVER_SOURCES})
  target_compile_definitions(bv2dedicated PRIVATE CONSOLE)
  bv2_game_target(bv2dedicated)
  add_dependencies(bv2dedicated bv2_seed_db)
endif()

if(BV2_BUILD_CLIENT)
  # Excluded from "all" until Step 3 replaces FMOD, DirectInput and the Win32/SDL1 window.
  add_executable(bv2 EXCLUDE_FROM_ALL ${BV2_GAME_SOURCES} src/ResIco.rc)
  bv2_game_target(bv2)
  target_link_libraries(bv2 PRIVATE zeven_client)
  if(WIN32)
    set_target_properties(bv2 PROPERTIES WIN32_EXECUTABLE ON)
  elseif(APPLE)
    set_target_properties(bv2 PROPERTIES MACOSX_BUNDLE ON)
  endif()
  add_dependencies(bv2 bv2_seed_db)
endif()
