file(GLOB_RECURSE COMMON common/*)
file(GLOB_RECURSE CRYPTO crypto/*)
file(GLOB_RECURSE CURRENCY_CORE currency_core/*)
file(GLOB_RECURSE CURRENCY_PROTOCOL currency_protocol/*)
file(GLOB_RECURSE DAEMON daemon/*)
file(GLOB_RECURSE P2P p2p/*)
file(GLOB_RECURSE RPC rpc/*)
file(GLOB_RECURSE SIMPLEWALLET simplewallet/*)
file(GLOB_RECURSE CONN_TOOL connectivity_tool/*)
file(GLOB_RECURSE WALLET wallet/*)
file(GLOB_RECURSE MINER miner/*)

source_group(common FILES ${COMMON})
source_group(crypto FILES ${CRYPTO})
source_group(currency_core FILES ${CURRENCY_CORE})
source_group(currency_protocol FILES ${CURRENCY_PROTOCOL})
source_group(daemon FILES ${DAEMON})
source_group(p2p FILES ${P2P})
source_group(rpc FILES ${RPC})
source_group(simplewallet FILES ${SIMPLEWALLET})
source_group(connectivity-tool FILES ${CONN_TOOL})
source_group(wallet FILES ${WALLET})
source_group(simpleminer FILES ${MINER})

add_library(common ${COMMON})
add_library(crypto ${CRYPTO})
add_library(currency_core ${CURRENCY_CORE})

add_library(rpc ${RPC})
add_dependencies(rpc version)

add_library(wallet ${WALLET})
add_dependencies(wallet version)

add_executable(daemon ${DAEMON} ${P2P} ${CURRENCY_PROTOCOL})
add_dependencies(daemon version)
target_link_libraries(daemon rpc currency_core crypto common
                             ${Boost_LIBRARIES})
                     
add_executable(connectivity_tool ${CONN_TOOL})
target_link_libraries(connectivity_tool currency_core crypto common
                                        ${Boost_LIBRARIES})

add_executable(simpleminer ${MINER})
target_link_libraries(simpleminer currency_core crypto common
                                  ${Boost_LIBRARIES})

add_executable(simplewallet ${SIMPLEWALLET})
add_dependencies(simplewallet version)
target_link_libraries(simplewallet wallet rpc currency_core crypto common
                                   ${Boost_LIBRARIES})

set_property(TARGET common crypto currency_core rpc wallet PROPERTY FOLDER "libs")
set_property(TARGET daemon simplewallet connectivity_tool simpleminer PROPERTY FOLDER "prog")
set_property(TARGET daemon PROPERTY OUTPUT_NAME "hpd")

if(APPLE)
  set(SIMPLE_BUNDLE 1)
endif()

if(SIMPLE_BUNDLE)
  set(INSTALL_DIR "${CMAKE_BINARY_DIR}/hp-${VERSION}")
  install(TARGETS daemon simplewallet connectivity_tool simpleminer
      RUNTIME DESTINATION "${INSTALL_DIR}" COMPONENT Runtime
  )

  install(FILES ${Boost_LIBRARIES} DESTINATION "${INSTALL_DIR}/lib")

  if(APPLE)
    set(FIXUP_COMMAND ${CMAKE_SOURCE_DIR}/utils/macosx_fixup.sh " " ${INSTALL_DIR})
    install(CODE "execute_process(COMMAND ${FIXUP_COMMAND})")
  endif()
endif()
