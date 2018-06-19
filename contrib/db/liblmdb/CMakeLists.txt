
set (lmdb_src_files mdb.c midl.c lmdb.h midl.h)

add_library(lmdb  ${lmdb_src_files})


if(WIN32)
  target_link_libraries(lmdb ntdll)
endif()
