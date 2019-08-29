#pragma once
#include "trees/regular_file.h"
#include "trees/regular_file_index.h"
#include <gsl/span>

namespace dogbox::tree
{
    size_t read_file(regular_file_index const &file, sqlite3 &database, regular_file::length_type const offset,
                     gsl::span<std::byte> const into);
}
