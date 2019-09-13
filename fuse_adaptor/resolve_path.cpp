#include "resolve_path.h"
#include "common/to_do.h"
#include "split_path.h"

namespace dogbox
{
    std::optional<tree::decoded_entry> find_entry(std::byte const *begin, std::byte const *end,
                                                  std::string_view const entry_name)
    {
        std::byte const *cursor = begin;
        while (cursor != end)
        {
            std::optional<std::tuple<tree::decoded_entry, std::byte const *>> const decode_result =
                tree::decode_entry(cursor, end);
            if (!decode_result)
            {
                TO_DO();
            }
            tree::decoded_entry const &entry = std::get<0>(*decode_result);
            if (entry.name == entry_name)
            {
                return entry;
            }
            cursor = std::get<1>(*decode_result);
        }
        return std::nullopt;
    }

    std::optional<directory_entry> resolve_path(sqlite3 &database, blob_hash_code const root,
                                                std::filesystem::path const &resolving)
    {
        path_split_result const split = split_path(resolving);
        if (split.head.empty())
        {
            return directory_entry{tree::entry_type::directory, root, 0};
        }
        std::optional<std::vector<std::byte>> const root_blob = load_blob(database, root);
        if (!root_blob)
        {
            TO_DO();
        }
        std::byte const *const begin = root_blob->data();
        std::byte const *const end = begin + root_blob->size();
        std::optional<tree::decoded_entry> const found_entry = find_entry(begin, end, split.head.string());
        if (!found_entry)
        {
            return std::nullopt;
        }
        tree::decoded_entry const &entry = *found_entry;
        switch (entry.type)
        {
        case tree::entry_type::directory:
            return resolve_path(database, entry.hash_code, split.tail);

        case tree::entry_type::regular_file:
            if (split.tail.empty())
            {
                return directory_entry{tree::entry_type::regular_file, entry.hash_code, entry.regular_file_size};
            }
            return std::nullopt;
        }
        TO_DO();
    }

    struct stat directory_entry_to_stat(directory_entry const entry)
    {
        struct stat status = {};
        switch (entry.type)
        {
        case tree::entry_type::regular_file:
            status.st_mode = S_IFREG | 0444;
            status.st_nlink = 1;
            status.st_size = entry.regular_file_size;
            break;

        case tree::entry_type::directory:
            status.st_mode = S_IFDIR | 0755;
            status.st_nlink = 2;
            break;
        }
        return status;
    }
}
