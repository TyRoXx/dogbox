file(GLOB sources *.h *.cpp)
add_library(fuse_adaptor_shared ${sources})
target_link_libraries(fuse_adaptor_shared trees blob_layer)
