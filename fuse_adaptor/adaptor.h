#pragma once

#include "channel.h"
#include "deleter.h"
#include "operations.h"
#include "user_data.h"

namespace dogbox::fuse
{
    struct adaptor
    {
        explicit adaptor(std::filesystem::path const &mount_point, sqlite3 &database, dogbox::blob_hash_code const root,
                         dogbox::tree::read_caching const read_cache_mode);

        struct fuse &get_fuse_handle() const;

    private:
        static fuse_args no_arguments;

        // fuse_unmount has to be called before fuse_destroy. That's why the destruction order of these handles is
        // important.
        dogbox::fuse::user_data user_data;
        std::unique_ptr<struct fuse, dogbox::fuse::fuse_deleter> fuse_handle;
        dogbox::fuse::channel channel;
    };
}
