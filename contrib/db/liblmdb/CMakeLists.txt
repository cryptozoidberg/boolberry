
if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wno-discarded-qualifiers -Wno-unused-but-set-variable")
    if ( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wimplicit-fallthrough=0")
    endif()
endif()

set (lmdb_src_files mdb.c midl.c lmdb.h midl.h)

add_library(lmdb  ${lmdb_src_files})


if(WIN32)
  target_link_libraries(lmdb ntdll)
endif()
