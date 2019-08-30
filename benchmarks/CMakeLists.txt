file(GLOB sources *.cpp)
add_executable(benchmarks ${sources})
target_link_libraries(benchmarks trees in_memory_fuse_frontend_shared fuse_adaptor benchmark::benchmark Boost::filesystem)
