# DukZeven: engine utilities behind the dk* APIs.
# dkt (textures) is in the core: the dedicated server still loads texture data through it.
add_library(zeven_core STATIC
  src/dkc.cpp
  src/dksvar.cpp
  src/CSystemVariable.cpp
  src/CString.cpp
  src/CVector.cpp
  src/CMatrix.cpp
  src/dkt.cpp
)
target_include_directories(zeven_core PUBLIC include PRIVATE src)
target_link_libraries(zeven_core PUBLIC Threads::Threads OpenGL::GL)
if(TARGET OpenGL::GLU)
  target_link_libraries(zeven_core PUBLIC OpenGL::GLU)
endif()
bv2_target_defaults(zeven_core)

# Client-only modules: window, input, GL, textures, fonts, particles, sound.
# Step 3 rewrites the platform parts (dkw, dki, dks) behind the same headers.
add_library(zeven_client STATIC
  src/dkw.cpp
  src/dki.cpp
  src/dkgl.cpp
  src/dkf.cpp
  src/CFont.cpp
  src/dkp.cpp
  src/CParticle.cpp
  src/dks.cpp
)
target_include_directories(zeven_client PUBLIC include PRIVATE src)
target_link_libraries(zeven_client PUBLIC zeven_core)
bv2_target_defaults(zeven_client)
