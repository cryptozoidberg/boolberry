cmake_minimum_required (VERSION 2.8.11)
project (x11)

add_library (x11hash aes_helper.c blake.c bmw.c cubehash.c echo.c fugue.c groestl.c hamsi.c hamsi_helper.c jh.c keccak.c luffa.c shavite.c simd.c skein.c x11.c)
target_include_directories (x11hash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(x11test x11test.c)
add_dependencies(x11test x11hash)
target_link_libraries (x11test LINK_PUBLIC x11hash)
