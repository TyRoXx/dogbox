#pragma once

#include "blob_layer/blob_storage.h"
#include "trees/open_file.h"
#include "trees/read_file.h"

namespace dogbox::fuse
{
    struct user_data
    {
        sqlite3 &database;
        blob_hash_code root;
        std::vector<std::optional<tree::open_file>> files;
        tree::read_caching read_cache_mode;
    };
}
