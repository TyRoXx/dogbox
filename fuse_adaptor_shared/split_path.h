#pragma once

#include <filesystem>

namespace dogbox
{
    struct path_split_result
    {
        std::filesystem::path head;
        std::filesystem::path tail;
    };

    path_split_result split_path(std::filesystem::path const &original);
}
