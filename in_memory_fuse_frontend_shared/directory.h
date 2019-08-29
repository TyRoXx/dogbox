#pragma once
#include "common/overloaded.h"
#include <filesystem>
#include <map>
#include <variant>
#include <vector>

namespace dogbox::in_memory_fuse
{
    struct regular_file
    {
        std::vector<std::byte> content;
    };

    inline bool operator==(regular_file const &left, regular_file const &right) noexcept
    {
        return left.content == right.content;
    }

    inline std::ostream &operator<<(std::ostream &out, regular_file const &printed)
    {
        return out << printed.content.size() << " bytes";
    }

    struct directory_entry;

    struct directory
    {
        std::map<std::string, directory_entry> entries;
    };

    std::ostream &operator<<(std::ostream &out, directory const &printed);

    struct directory_entry
    {
        std::variant<regular_file, directory> content;
    };

    inline bool operator==(directory_entry const &left, directory_entry const &right) noexcept
    {
        return left.content == right.content;
    }

    std::ostream &operator<<(std::ostream &out, directory_entry const &printed);

    inline bool operator==(directory const &left, directory const &right) noexcept
    {
        return left.entries == right.entries;
    }

    regular_file scan_regular_file(std::filesystem::path const &input);
    directory scan_directory(std::filesystem::path const &input);
}
