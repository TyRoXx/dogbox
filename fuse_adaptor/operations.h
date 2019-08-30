#pragma once

#include "blob_layer/blob_storage.h"
#include "trees/regular_file_index.h"
#include <filesystem>
#include <fuse.h>
#include <sqlite3.h>

namespace dogbox::fuse
{
    int adaptor_getattr(const char *const request_path, struct stat *const into);

    int adaptor_readdir(const char *request_path, void *buf, fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info *file);

    int adaptor_open(const char *request_path, struct fuse_file_info *file);

    int adaptor_release(const char *request_path, struct fuse_file_info *file);

    int adaptor_read(const char *request_path, char *const into, size_t const size, off_t const offset,
                     struct fuse_file_info *const file);

    constexpr fuse_operations make_operations() noexcept
    {
        fuse_operations result = {};
        result.getattr = adaptor_getattr;
        result.readdir = adaptor_readdir;
        result.open = adaptor_open;
        result.release = adaptor_release;
        result.read = adaptor_read;
        return result;
    }

    constexpr fuse_operations operations = dogbox::fuse::make_operations();
}
