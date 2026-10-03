# DukZeven: engine utilities behind the dk* APIs.

# glad: OpenGL 2.1 compatibility loader (+ GL_EXT_bgra), generated with
#   glad --api gl:compatibility=2.1 --extensions GL_EXT_bgra --out-path engine/zeven/third_party/glad c
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
bv2_target_defaults(glad)

# GLU stays on the system library for now (gluPerspective, gluBuild2DMipmaps, quadrics); GL itself comes from glad.
add_library(bv2_glu INTERFACE)
target_link_libraries(bv2_glu INTERFACE OpenGL::GL)
if(TARGET OpenGL::GLU)
  target_link_libraries(bv2_glu INTERFACE OpenGL::GLU)
endif()

# Shared by every executable.
add_library(zeven_core STATIC
  src/dkc.cpp
  src/dksvar.cpp
  src/CSystemVariable.cpp
  src/CString.cpp
  src/CVector.cpp
  src/CMatrix.cpp
)
target_include_directories(zeven_core PUBLIC include PRIVATE src)
target_link_libraries(zeven_core PUBLIC Threads::Threads glad)
bv2_target_defaults(zeven_core)
bv2_target_strict(zeven_core)

# Headless server: there is no GL context, so dkt gets no-op stand-ins.
add_library(zeven_console STATIC src/dkt_console.cpp)
target_link_libraries(zeven_console PUBLIC zeven_core)
bv2_target_defaults(zeven_console)
bv2_target_strict(zeven_console)

if(BV2_BUILD_CLIENT)
  # Client modules: window, input, GL state, textures, fonts, particles, sound. SDL3, miniaudio and stb come from vcpkg.
  find_package(SDL3 CONFIG REQUIRED)
  find_path(BV2_MINIAUDIO_INCLUDE_DIR miniaudio.h PATH_SUFFIXES miniaudio)
  if(NOT BV2_MINIAUDIO_INCLUDE_DIR)
    message(FATAL_ERROR "miniaudio.h not found (vcpkg port miniaudio, or brew install miniaudio)")
  endif()
  # OGG music needs stb_vorbis.c (vcpkg port stb). Without it .ogg files do not play.
  find_path(BV2_STB_VORBIS_DIR stb_vorbis.c PATH_SUFFIXES stb)

  add_library(zeven_client STATIC
    src/dkw.cpp
    src/dki.cpp
    src/dkgl.cpp
    src/dkt.cpp
    src/dkf.cpp
    src/CFont.cpp
    src/dkp.cpp
    src/CParticle.cpp
    src/dks.cpp
  )
  target_include_directories(zeven_client PUBLIC include PRIVATE src "${BV2_MINIAUDIO_INCLUDE_DIR}")
  target_link_libraries(zeven_client PUBLIC zeven_core bv2_glu SDL3::SDL3 ${CMAKE_DL_LIBS})
  if(BV2_STB_VORBIS_DIR)
    target_include_directories(zeven_client PRIVATE "${BV2_STB_VORBIS_DIR}")
    target_compile_definitions(zeven_client PRIVATE BV2_HAVE_STB_VORBIS)
  else()
    message(STATUS "stb_vorbis.c not found: OGG music will not play")
  endif()
  # dks.cpp compiles the miniaudio and stb_vorbis implementations; their warnings are not ours.
  if(MSVC)
    set_source_files_properties(src/dks.cpp PROPERTIES COMPILE_OPTIONS "/W0")
  else()
    set_source_files_properties(src/dks.cpp PROPERTIES COMPILE_OPTIONS "-w")
  endif()
  bv2_target_defaults(zeven_client)
  bv2_target_strict(zeven_client)
endif()
