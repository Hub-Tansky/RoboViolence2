# dko: .DKO model loader and ray/sphere intersection.
add_library(dko STATIC
  src/CdkoAnimation.cpp
  src/CdkoMaterial.cpp
  src/CdkoMesh.cpp
  src/CdkoModel.cpp
  src/CFace.cpp
  src/COctree.cpp
  src/dko.cpp
  src/eHierarchic.cpp
  src/ePTexture.cpp
)
target_include_directories(dko PUBLIC include PRIVATE src)
target_link_libraries(dko PUBLIC zeven_core bv2_glu)
bv2_target_defaults(dko)
bv2_target_strict(dko)
