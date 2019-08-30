#pragma once

#include "blob_layer/blob_storage.h"
#include "trees/regular_file_index.h"

namespace dogbox::fuse
{
    struct open_file
    {
        blob_hash_code hash_code;
        std::optional<tree::regular_file_index> index;
    };
}
