file(GLOB sources *.h *.cpp ../common/*)
add_executable(tests ${sources})
target_link_libraries(tests in_memory_fuse_frontend_shared fuse_read_only_adaptor Boost::unit_test_framework Boost::filesystem)
