#pragma once

#include "blob_layer/blob_storage.h"
#include "trees/regular_file_index.h"
#include <filesystem>
#include <fuse.h>
#include <sqlite3.h>

namespace dogbox::fuse
{
    struct channel
    {
        fuse_chan *handle;
        std::filesystem::path mount_point;

        channel() noexcept
        {
        }

        channel(fuse_chan &handle_, std::filesystem::path mount_point_) noexcept
            : handle(&handle_)
            , mount_point(std::move(mount_point_))
        {
        }

        ~channel() noexcept
        {
            if (!handle)
            {
                return;
            }
            fuse_unmount(mount_point.c_str(), handle);
        }

        channel(channel &&other) noexcept
            : handle(other.handle)
            , mount_point(std::move(other.mount_point))
        {
            other.handle = nullptr;
            other.mount_point = "";
        }

        channel &operator=(channel &&other) noexcept
        {
            using std::swap;
            swap(handle, other.handle);
            swap(mount_point, other.mount_point);
            return *this;
        }
    };

    struct fuse_deleter
    {
        void operator()(struct fuse *const handle) const noexcept
        {
            fuse_destroy(handle);
        }
    };

    struct open_file
    {
        blob_hash_code hash_code;
        std::optional<tree::regular_file_index> index;
    };

    struct user_data
    {
        sqlite3 &database;
        blob_hash_code root;
        std::vector<std::optional<open_file>> files;
    };

    fuse_operations make_operations() noexcept;
}
