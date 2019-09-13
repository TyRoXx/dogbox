file(GLOB sources *.h *.cpp)
add_library(fuse_write_adaptor ${sources})
target_link_libraries(fuse_write_adaptor fuse_adaptor_shared ${FUSE_LIB})
