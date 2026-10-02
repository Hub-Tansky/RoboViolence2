# babonet: TCP/UDP networking (bb_* API) and the CThread wrapper.
add_library(babonet STATIC
  src/baboNet.cpp
  src/cClient.cpp
  src/cConnection.cpp
  src/cDNSquery.cpp
  src/cIncConnection.cpp
  src/cPacket.cpp
  src/cPeer.cpp
  src/cPeer2Peer.cpp
  src/cServer.cpp
  src/cUDPpacket.cpp
  src/md5c.c
  src/md5class.cpp
  src/CThread.cpp
)
target_include_directories(babonet PUBLIC include PRIVATE src)
target_link_libraries(babonet PUBLIC zeven_core Threads::Threads)
if(WIN32)
  target_link_libraries(babonet PUBLIC ws2_32 iphlpapi)
endif()
bv2_target_defaults(babonet)
