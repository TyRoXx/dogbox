#pragma once

#include "blob_layer/blob_storage.h"
#include "regular_file.h"
#include <vector>

namespace dogbox::tree
{
    struct regular_file_index
    {
        std::vector<blob_hash_code> pieces;
        std::vector<std::byte> tail;
    };

    inline regular_file::length_type file_size(regular_file_index const &file)
    {
        regular_file::length_type size = (file.pieces.size() * regular_file::piece_length) + file.tail.size();
        return size;
    }

    regular_file_index load_regular_file_index(sqlite3 &database, blob_hash_code const hash_code);
}
