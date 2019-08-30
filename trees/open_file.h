#pragma once

#include "blob_layer/blob_storage.h"
#include "trees/regular_file_index.h"

namespace dogbox::tree
{
    struct open_file
    {
        blob_hash_code hash_code;
        std::optional<regular_file_index> index;
        std::optional<uint64_t> cached_piece;
        std::vector<std::byte> read_cache;
    };
}
