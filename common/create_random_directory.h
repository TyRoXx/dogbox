#pragma once

#include "create_random_file.h"

namespace dogbox
{
    inline void create_random_directory(std::filesystem::path const &directory, uint64_t const number_of_files,
                                        uint64_t const total_file_size)
    {
        std::filesystem::remove_all(directory);
        std::filesystem::create_directories(directory);
        for (uint64_t i = 0; i < number_of_files; ++i)
        {
            size_t const file_size = total_file_size / number_of_files;
            create_random_file((directory / std::to_string(i)), file_size);
        }
    }
}
