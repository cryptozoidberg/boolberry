file(GLOB_RECURSE COMMON common/*)
file(GLOB_RECURSE CRYPTO crypto/*)
file(GLOB_RECURSE CURRENCY_CORE currency_core/*)
file(GLOB_RECURSE CURRENCY_PROTOCOL currency_protocol/*)
file(GLOB_RECURSE DAEMON daemon/*)
# file(GLOB_RECURSE SIMPLEMINER miner/*)


file(GLOB_RECURSE P2P p2p/*)
file(GLOB_RECURSE RPC rpc/*)
file(GLOB_RECURSE SIMPLEWALLET simplewallet/*)
file(GLOB_RECURSE CONN_TOOL connectivity_tool/*)
file(GLOB_RECURSE WALLET wallet/*)
file(GLOB_RECURSE MINER miner/*)

if(BUILD_GUI)
  if(MSVC)
    file(GLOB_RECURSE QTDAEMON gui/qt-daemon/*.cpp gui/qt-daemon/*.h gui/qt-daemon/app.rc)
  else()
    file(GLOB_RECURSE QTDAEMON gui/qt-daemon/*.cpp gui/qt-daemon/*.h)    
  endif()
endif()


source_group(common FILES ${COMMON})
source_group(crypto FILES ${CRYPTO})
source_group(currency_core FILES ${CURRENCY_CORE})
source_group(currency_protocol FILES ${CURRENCY_PROTOCOL})
source_group(daemon FILES ${DAEMON})
source_group(p2p FILES ${P2P})
source_group(rpc FILES ${RPC})
source_group(simplewallet FILES ${SIMPLEWALLET})
# source_group(simpleminer FILES ${SIMPLEMINER})
source_group(connectivity-tool FILES ${CONN_TOOL})
source_group(wallet FILES ${WALLET})

if(BUILD_GUI)
  source_group(qtdaemon FILES ${QTDAEMON})
endif()


add_library(common ${COMMON})
add_library(crypto ${CRYPTO})

add_library(currency_core ${CURRENCY_CORE})
add_dependencies(currency_core version)
target_link_libraries(currency_core lmdb)

add_library(rpc ${RPC})
add_dependencies(rpc version)

add_library(wallet ${WALLET})
add_dependencies(wallet version)

add_executable(daemon ${DAEMON} ${P2P} ${CURRENCY_PROTOCOL})
add_dependencies(daemon version)
target_link_libraries(daemon rpc currency_core crypto common upnpc-static ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
                     
add_executable(connectivity_tool ${CONN_TOOL})
add_dependencies(connectivity_tool version)
target_link_libraries(connectivity_tool currency_core crypto common ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})


add_executable(simplewallet ${SIMPLEWALLET})
add_dependencies(simplewallet version)
target_link_libraries(simplewallet wallet rpc currency_core crypto common ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

# add_executable(simpleminer ${SIMPLEMINER})
# add_dependencies(simpleminer version)
# target_link_libraries(simpleminer currency_core crypto common ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})

set_property(TARGET common crypto currency_core rpc wallet PROPERTY FOLDER "libs")
set_property(TARGET daemon simplewallet connectivity_tool PROPERTY FOLDER "prog")
set_property(TARGET daemon PROPERTY OUTPUT_NAME "boolbd")

if(BUILD_GUI)
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  SET(MACOSX_BUNDLE_ICON_FILE app.icns)
  add_executable(Boolberry WIN32 MACOSX_BUNDLE ${QTDAEMON} )	  
  QT5_USE_MODULES(Boolberry WebKit WebKitWidgets)
  target_link_libraries(Boolberry wallet rpc currency_core crypto common Qt5::Widgets ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} )
  set_property(TARGET Boolberry PROPERTY FOLDER "prog")
  set(CMAKE_AUTOMOC OFF)
endif()

if(APPLE)
  set(SIMPLE_BUNDLE 1)
endif()

if(SIMPLE_BUNDLE)
  set(INSTALL_DIR "${CMAKE_BINARY_DIR}/hp-${VERSION}")
  install(TARGETS daemon simplewallet connectivity_tool
      RUNTIME DESTINATION "${INSTALL_DIR}" COMPONENT Runtime
  )

  install(FILES ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} DESTINATION "${INSTALL_DIR}/lib")

  if(APPLE)
    set(FIXUP_COMMAND ${CMAKE_SOURCE_DIR}/utils/macosx_fixup.sh " " ${INSTALL_DIR})
    install(CODE "execute_process(COMMAND ${FIXUP_COMMAND})")
  endif()
endif()
