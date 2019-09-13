#pragma once

#include "blob_layer/blob_storage.h"
#include "trees/directory.h"
#include "trees/regular_file.h"
#include <filesystem>

namespace dogbox
{
    struct directory_entry
    {
        tree::entry_type type;
        blob_hash_code hash_code;
        regular_file::length_type regular_file_size;
    };

    std::optional<tree::decoded_entry> find_entry(std::byte const *begin, std::byte const *end,
                                                  std::string_view const entry_name);

    std::optional<directory_entry> resolve_path(sqlite3 &database, blob_hash_code const root,
                                                std::filesystem::path const &resolving);
}
