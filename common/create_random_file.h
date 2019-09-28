#pragma once

#include "file_descriptor.h"
#include <filesystem>
#include <gsl/span>
#include <random>

namespace dogbox
{
    inline void create_random_file(std::filesystem::path const &file, uint64_t const size)
    {
#if DOGBOX_HAS_FILE_DESCRIPTOR
        file_descriptor const created = create_file(file).value();
        file_descriptor const random = open_file_for_reading("/dev/urandom").value();
        std::array<std::byte, 0x10000> buffer;
        uint64_t written = 0;
        while (written < size)
        {
            size_t const reading = static_cast<size_t>(std::min<uint64_t>(buffer.size(), (size - written)));
            ssize_t const read_result = read(random.handle, buffer.data(), reading);
            if (read_result < 0)
            {
                TO_DO();
            }
            ssize_t const write_result = write(created.handle, buffer.data(), reading);
            if (write_result < 0)
            {
                TO_DO();
            }
            written += reading;
        }
		#else
		TO_DO();
		#endif
    }
}
