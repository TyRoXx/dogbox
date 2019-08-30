#pragma once
#include "common/to_do.h"
#include "open_file.h"
#include "trees/regular_file.h"
#include <gsl/span>

namespace dogbox::tree
{
    enum class read_caching
    {
        none,
        one_piece
    };

    inline std::ostream &operator<<(std::ostream &out, read_caching const printed)
    {
        switch (printed)
        {
        case read_caching::none:
            return out << "none";
        case read_caching::one_piece:
            return out << "one_piece";
        }
        TO_DO();
    }

    constexpr dogbox::tree::read_caching all_read_caching_modes[] = {
        dogbox::tree::read_caching::none, dogbox::tree::read_caching::one_piece};

    size_t read_file(open_file &file, sqlite3 &database, regular_file::length_type const offset,
                     gsl::span<std::byte> const into, read_caching const caching);
}
