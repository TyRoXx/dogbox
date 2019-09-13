#!/usr/bin/env bash
find benchmarks blob_layer fuse_read_only_adaptor fuse_adaptor_shared in_memory_fuse_frontend in_memory_fuse_frontend_shared tests trees common -iname '*.h' -o -iname '*.cpp' | xargs clang-format-3.9 -i
