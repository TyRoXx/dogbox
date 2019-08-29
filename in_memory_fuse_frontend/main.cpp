#include "common/overloaded.h"
#include "common/to_do.h"
#include "in_memory_fuse_frontend_shared/directory.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <functional>
#include <fuse.h>
#include <iostream>
#include <mutex>

namespace dogbox::in_memory_fuse
{
    struct our_fuse_user_data
    {
        std::filesystem::path input_dir;
        std::optional<directory> tree;
        std::mutex mutex;

        directory &require_tree()
        {
            std::scoped_lock<std::mutex> lock(mutex);
            if (!tree)
            {
                tree = scan_directory(input_dir);
            }
            return *tree;
        }
    };

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

    std::optional<struct stat> getattr_impl(std::filesystem::path const request_path_parsed,
                                            directory const &current_root)
    {
        auto[root_path, relative_path] = split_path(request_path_parsed);
        if (root_path.empty())
        {
            struct stat result;
            std::memset(&result, 0, sizeof(result));
            result.st_mode = S_IFDIR | 0755;
            result.st_nlink = 2;
            return result;
        }
        auto const entry_found = current_root.entries.find(root_path.string());
        if (entry_found == current_root.entries.end())
        {
            return std::nullopt;
        }

        auto on_regular_file = [&](regular_file const &regular) -> std::optional<struct stat>
        {
            struct stat result;
            std::memset(&result, 0, sizeof(result));
            result.st_mode = S_IFREG | 0444;
            result.st_nlink = 1;
            result.st_size = static_cast<off64_t>(regular.content.size());
            return result;
        };
        auto on_directory = [&](directory const &subdirectory) -> std::optional<struct stat>
        {
            return getattr_impl(relative_path, subdirectory);
        };
        return std::visit(overloaded{on_regular_file, on_directory}, entry_found->second.content);
    }

    int hello_getattr(const char *request_path, struct stat *stbuf)
    {
        memset(stbuf, 0, sizeof(*stbuf));
        fuse_context *const fuse = fuse_get_context();
        our_fuse_user_data *const data = static_cast<our_fuse_user_data *>(fuse->private_data);
        directory &current_root = data->require_tree();
        assert(request_path[0] == '/');
        std::filesystem::path request_path_parsed = request_path + 1;
        if (auto result = getattr_impl(request_path_parsed, current_root); result)
        {
            *stbuf = *result;
            return 0;
        }
        return -ENOENT;
    }

    using directory_entry_handler = std::function<void(char const *)>;

    void list_directory(directory const &current_root, directory_entry_handler const &add_entry)
    {
        add_entry(".");
        add_entry("..");
        for (auto const &entry : current_root.entries)
        {
            add_entry(entry.first.c_str());
        }
    }

    int readdir_impl(std::filesystem::path const request_path_parsed, directory const &current_root,
                     directory_entry_handler const &add_entry)
    {
        auto[root_path, relative_path] = split_path(request_path_parsed);
        if (root_path.empty())
        {
            list_directory(current_root, add_entry);
            return 0;
        }
        auto const entry_found = current_root.entries.find(root_path.string());
        if (entry_found == current_root.entries.end())
        {
            return -ENOENT;
        }

        return std::visit(overloaded{[&](regular_file const &) -> int { TO_DO(); },
                                     [&](directory const &subdirectory) -> int {
                                         return readdir_impl(relative_path, subdirectory, add_entry);
                                     }},
                          entry_found->second.content);
    }

    int hello_readdir(const char *request_path, void *buf, fuse_fill_dir_t filler, off_t offset,
                      struct fuse_file_info *fi)
    {
        (void)offset;
        (void)fi;
        fuse_context *const fuse = fuse_get_context();
        our_fuse_user_data *const data = static_cast<our_fuse_user_data *>(fuse->private_data);
        directory &current_root = data->require_tree();
        assert(request_path[0] == '/');
        std::filesystem::path request_path_parsed = request_path + 1;
        return readdir_impl(request_path_parsed, current_root,
                            [buf, filler](char const *const name) { filler(buf, name, nullptr, 0); });
    }

    struct open_result
    {
        int return_code;
        unsigned long file_handle;
    };

    open_result open_impl(std::filesystem::path const request_path_parsed, directory const &current_root,
                          int const flags)
    {
        auto[root_path, relative_path] = split_path(request_path_parsed);
        auto const entry_found = current_root.entries.find(root_path.string());
        if (entry_found == current_root.entries.end())
        {
            return open_result{-ENOENT, 0};
        }

        if ((flags & 3) != O_RDONLY)
        {
            return open_result{-EACCES, 0};
        }

        return std::visit(overloaded{[&](regular_file const &regular) -> open_result {
                                         if (relative_path.empty())
                                         {
                                             return open_result{0, reinterpret_cast<unsigned long>(&regular)};
                                         }
                                         TO_DO();
                                     },
                                     [&](directory const &subdirectory) -> open_result {
                                         if (relative_path.empty())
                                         {
                                             TO_DO();
                                         }
                                         return open_impl(relative_path, subdirectory, flags);
                                     }},
                          entry_found->second.content);
    }

    int hello_open(const char *request_path, struct fuse_file_info *fi)
    {
        fuse_context *const fuse = fuse_get_context();
        our_fuse_user_data *const data = static_cast<our_fuse_user_data *>(fuse->private_data);
        directory &current_root = data->require_tree();
        assert(request_path[0] == '/');
        std::filesystem::path request_path_parsed = request_path + 1;
        open_result const result = open_impl(request_path_parsed, current_root, fi->flags);
        fi->fh = result.file_handle;
        return result.return_code;
    }

    int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
    {
        (void)path;
        regular_file const &regular = *reinterpret_cast<regular_file const *>(fi->fh);
        auto const &content = regular.content;
        if (offset > static_cast<off_t>(content.size()))
        {
            offset = static_cast<off_t>(content.size());
        }
        size_t const actual_read_size = (std::min<size_t>)(size, (content.size() - static_cast<size_t>(offset)));
        std::memcpy(buf, content.data() + offset, actual_read_size);
        return static_cast<int>(actual_read_size);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Command line arguments: INPUT_DIR [FUSE OPTIONS]...\n";
        return 1;
    }
    std::filesystem::path const input_dir = argv[1];
    struct fuse_operations hello_oper = {};
    hello_oper.getattr = dogbox::in_memory_fuse::hello_getattr;
    hello_oper.readdir = dogbox::in_memory_fuse::hello_readdir;
    hello_oper.open = dogbox::in_memory_fuse::hello_open;
    hello_oper.read = dogbox::in_memory_fuse::hello_read;
    dogbox::in_memory_fuse::our_fuse_user_data user_data{input_dir, std::nullopt, {}};
    return fuse_main(argc - 1, argv + 1, &hello_oper, &user_data);
}
