#include "import.h"
#include "common/file_descriptor.h"
#include "common/to_do.h"
#include "directory.h"
#include "regular_file.h"
#include <boost/range/irange.hpp>
#include <fstream>
#include <future>
#include <thread>

namespace dogbox::import
{
    std::ostream &operator<<(std::ostream &out, parallelism const printed)
    {
        switch (printed)
        {
        case parallelism::full:
            return out << "full";
        case parallelism::none:
            return out << "none";
        }
        return out << "???";
    }

    blob_hash_code from_filesystem_directory(sqlite3 &database, std::filesystem::path const &root,
                                             parallelism const parallel)
    {
        // TODO: sort entries (for example by name)
        std::vector<std::byte> encoded;
        std::filesystem::directory_iterator i(root);
        for (; i != std::filesystem::directory_iterator(); ++i)
        {
            auto entry_path = i->path();
            auto entry_name = entry_path.filename();
            switch (i->status().type())
            {
            case std::filesystem::file_type::regular:
            {
                regular_file_imported const regular = from_filesystem_regular_file(database, entry_path, parallel);
                tree::encode_entry(tree::entry_type::regular_file, entry_name.string(), regular.hash_code,
                                   regular.content_size, std::back_inserter(encoded));
                break;
            }

            case std::filesystem::file_type::directory:
            {
                blob_hash_code const subdirectory = from_filesystem_directory(database, entry_path, parallel);
                tree::encode_entry(
                    tree::entry_type::directory, entry_name.string(), subdirectory, 0, std::back_inserter(encoded));
                break;
            }

            case std::filesystem::file_type::none:
            case std::filesystem::file_type::not_found:
            case std::filesystem::file_type::symlink:
            case std::filesystem::file_type::block:
            case std::filesystem::file_type::character:
            case std::filesystem::file_type::fifo:
            case std::filesystem::file_type::socket:
            case std::filesystem::file_type::unknown:
                break;
            }
        }

        return store_blob(database, encoded.data(), encoded.size());
    }

    namespace
    {
        template <class Unsigned, class Action>
        void parallel_for(Unsigned begin, Unsigned count, size_t const max_paralellism, Action &&action)
        {
            assert(max_paralellism >= 1);
            if (count == 0)
            {
                return;
            }
            size_t const actual_parallelism = std::min(max_paralellism, count);
            assert(actual_parallelism >= 1);
            size_t const worker_count = actual_parallelism - 1;
            std::vector<std::future<void>> workers(worker_count);
            for (size_t i = 0; i < worker_count; ++i)
            {
                workers[i] = std::async(std::launch::async, [begin, i, action, count, actual_parallelism]() {
                    for (Unsigned k = i; k < count; k += actual_parallelism)
                    {
                        action(begin + k);
                    }
                });
            }
            for (Unsigned k = worker_count; k < count; k += actual_parallelism)
            {
                action(begin + k);
            }
            for (auto &worker : workers)
            {
                worker.get();
            }
        }
    }

    regular_file_imported from_filesystem_regular_file(sqlite3 &database, std::filesystem::path const &input,
                                                       parallelism const parallel)
    {
        file_descriptor const file = open_file_for_reading(input).value();
        off_t const size_result = lseek64(file.handle, 0, SEEK_END);
        if (size_result < 0)
        {
            TO_DO();
        }
        uint64_t const size = static_cast<uint64_t>(size_result);
        if (lseek64(file.handle, 0, SEEK_SET) < 0)
        {
            TO_DO();
        }
        std::vector<std::byte> encoded;
        size_t const number_of_pieces = (size / regular_file::piece_length);
        uint64_t const remaining_size = (size - (number_of_pieces * regular_file::piece_length));
        size_t concurrency = std::thread::hardware_concurrency();
        switch (parallel)
        {
        case parallelism::full:
            break;

        case parallelism::none:
            concurrency = 1;
            break;
        }
        {
            std::vector<blob_hash_code> hash_codes(number_of_pieces);
            std::mutex database_mutex;
            parallel_for<size_t>(0, number_of_pieces, (number_of_pieces >= (concurrency * 2)) ? concurrency : 1,
                                 [&](size_t const index) {
                                     std::vector<std::byte> piece_buffer(regular_file::piece_length);
                                     read_at(file.handle, (index * regular_file::piece_length), piece_buffer.data(),
                                             piece_buffer.size());
                                     blob_hash_code const piece_hash_code = [&]() {
                                         std::scoped_lock<std::mutex> lock(database_mutex);
                                         return store_blob(database, piece_buffer.data(), piece_buffer.size());
                                     }();
                                     hash_codes[index] = piece_hash_code;
                                 });
            for (auto const piece_hash_code : hash_codes)
            {
                regular_file::encode_piece(piece_hash_code, std::back_inserter(encoded));
            }
        }

        std::vector<std::byte> piece_buffer(remaining_size);
        read_at(file.handle, number_of_pieces * regular_file::piece_length, piece_buffer.data(), piece_buffer.size());
        regular_file::finish_encoding(piece_buffer.data(), remaining_size, std::back_inserter(encoded));
        return regular_file_imported{store_blob(database, encoded.data(), encoded.size()), size};
    }
}
