#pragma once

#include "file_descriptor.h"
#include <filesystem>
#include <gsl/span>
#include <random>

namespace dogbox
{
    inline void create_randomness(gsl::span<std::byte> const into)
    {
        file_descriptor const random = open_file_for_reading("/dev/urandom").value();
        ptrdiff_t written = 0;
        while (written < into.size())
        {
            size_t const reading = static_cast<size_t>(into.size() - written);
            ssize_t const read_result = read(random.handle, into.data() + written, reading);
            if (read_result < 0)
            {
                TO_DO();
            }
            written += reading;
        }
    }
}
