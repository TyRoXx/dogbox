#include "operations.h"
#include "common/to_do.h"
#include "trees/directory.h"
#include "trees/read_file.h"
#include "user_data.h"
#include <algorithm>
#include <cstring>

namespace dogbox::fuse
{
    namespace
    {
        struct path_split_result
        {
            std::filesystem::path head;
            std::filesystem::path tail;
        };

        path_split_result split_path(std::filesystem::path const &original)
        {
            auto const &original_string = original.string();
            auto const slash = std::find(original_string.begin(), original_string.end(), '/');
            if (slash == original_string.end())
            {
                return path_split_result{original_string, ""};
            }
            return path_split_result{std::filesystem::path(original_string.begin(), slash),
                                     std::filesystem::path(slash + 1, original_string.end())};
        }

        struct directory_entry
        {
            tree::entry_type type;
            blob_hash_code hash_code;
            regular_file::length_type regular_file_size;
        };

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

    int adaptor_getattr(const char *const request_path, struct stat *const into)
    {
        fuse_context &fuse = *fuse_get_context();
        user_data &user = *static_cast<user_data *>(fuse.private_data);
        std::optional<directory_entry> const resolved = resolve_path(user.database, user.root, request_path + 1);
        if (!resolved)
        {
            return -ENOENT;
        }
        *into = directory_entry_to_stat(*resolved);
        return 0;
    }

    int adaptor_readdir(const char *request_path, void *buf, fuse_fill_dir_t filler, off_t offset,
                        struct fuse_file_info *file)
    {
        // TODO: use offset
        (void)offset;
        (void)file;

        fuse_context &fuse = *fuse_get_context();
        user_data &user = *static_cast<user_data *>(fuse.private_data);
        std::optional<directory_entry> const resolved = resolve_path(user.database, user.root, request_path + 1);
        if (!resolved)
        {
            return -ENOENT;
        }

        switch (resolved->type)
        {
        case tree::entry_type::directory:
        {
            std::optional<std::vector<std::byte>> const root_blob = load_blob(user.database, resolved->hash_code);
            if (!root_blob)
            {
                TO_DO();
            }
            std::byte const *const begin = root_blob->data();
            std::byte const *const end = begin + root_blob->size();
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
                struct stat const status =
                    directory_entry_to_stat(directory_entry{entry.type, entry.hash_code, entry.regular_file_size});
                filler(buf, std::string(entry.name).c_str(), &status, 0 /*TODO use offset*/);
                cursor = std::get<1>(*decode_result);
            }
            return 0;
        }

        case tree::entry_type::regular_file:
            TO_DO();
        }
        TO_DO();
    }

    int adaptor_open(const char *request_path, struct fuse_file_info *file)
    {
        fuse_context &fuse = *fuse_get_context();
        user_data &user = *static_cast<user_data *>(fuse.private_data);
        std::optional<directory_entry> const resolved = resolve_path(user.database, user.root, request_path + 1);
        if (!resolved)
        {
            return -ENOENT;
        }
        switch (resolved->type)
        {
        case tree::entry_type::directory:
            return -EISDIR;

        case tree::entry_type::regular_file:
            for (size_t i = 0; i < user.files.size(); ++i)
            {
                std::optional<tree::open_file> &entry = user.files[i];
                if (!entry)
                {
                    entry = tree::open_file{
                        resolved->regular_file_size, resolved->hash_code, std::nullopt, std::nullopt, {}};
                    file->fh = i;
                    return 0;
                }
            }
            file->fh = user.files.size();
            user.files.emplace_back(
                tree::open_file{resolved->regular_file_size, resolved->hash_code, std::nullopt, std::nullopt, {}});
            return 0;
        }
        TO_DO();
    }

    int adaptor_release(const char *request_path, struct fuse_file_info *file)
    {
        (void)request_path;
        fuse_context &fuse = *fuse_get_context();
        user_data &user = *static_cast<user_data *>(fuse.private_data);
        user.files[file->fh] = std::nullopt;
        return 0;
    }

    int adaptor_read(const char *request_path, char *const into, size_t const size, off_t const offset,
                     struct fuse_file_info *const file)
    {
        (void)request_path;
        fuse_context &fuse = *fuse_get_context();
        user_data &user = *static_cast<user_data *>(fuse.private_data);
        assert(file->fh < user.files.size());
        tree::open_file &opened = *user.files[file->fh];
        if (!opened.index)
        {
            opened.index = tree::load_regular_file_index(user.database, opened.file_size, opened.hash_code);
        }
        assert(opened.index);
        // TODO handle overflow
        return static_cast<int>(read_file(opened, user.database, offset,
                                          gsl::span<std::byte>(reinterpret_cast<std::byte *>(into), size),
                                          user.read_cache_mode));
    }
}
