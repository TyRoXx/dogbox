#pragma once

#include "blob_layer/blob_storage.h"
#include "open_file.h"

namespace dogbox::fuse
{
    struct user_data
    {
        sqlite3 &database;
        blob_hash_code root;
        std::vector<std::optional<open_file>> files;
    };
}
