file(GLOB sources *.h *.cpp)
add_library(fuse_read_only_adaptor ${sources})
target_link_libraries(fuse_read_only_adaptor fuse_adaptor_shared ${FUSE_LIB})
