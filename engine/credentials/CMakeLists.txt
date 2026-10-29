# credentials: HMAC-SHA256 proofs and nonces for passwords that never cross the wire (libsodium, ADR 0014, 0038).
add_library(credentials STATIC src/Credentials.cpp)
target_include_directories(credentials PUBLIC include)
target_link_libraries(credentials PUBLIC unofficial-sodium::sodium)
bv2_target_defaults(credentials)
bv2_target_strict(credentials)
