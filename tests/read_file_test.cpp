#include "common/create_randomness.h"
#include "trees/import.h"
#include "trees/read_file.h"
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>

namespace
{
    std::vector<std::byte> generate_randomness(size_t const size)
    {
        std::vector<std::byte> result(size);
        dogbox::create_randomness(result);
        return result;
    }

    void write_file(std::filesystem::path const file, gsl::span<std::byte const> const content)
    {
        std::ofstream opened(file.string(), std::ios::binary);
        opened.write(reinterpret_cast<char const *>(content.data()), content.size());
        if (!opened)
        {
            TO_DO();
        }
    }
}

BOOST_DATA_TEST_CASE(read_file, boost::unit_test::data::make<size_t>({dogbox::regular_file::piece_length + 1, 0, 1,
                                                                      4096, dogbox::regular_file::piece_length,
                                                                      dogbox::regular_file::piece_length - 1}) *
                                    boost::unit_test::data::make<size_t>({dogbox::regular_file::piece_length - 1, 4096,
                                                                          dogbox::regular_file::piece_length}),
                     file_size, read_size)
{
    dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
    dogbox::initialize_blob_storage(*database);
    BOOST_TEST(dogbox::count_blobs(*database) == 0);
    std::filesystem::path const file = "/tmp/dogbox_regular_file";
    std::vector<std::byte> const file_content = generate_randomness(file_size);
    write_file(file, file_content);
    dogbox::import::regular_file_imported const test_hash_code =
        dogbox::import::from_filesystem_regular_file(*database, file, dogbox::import::parallelism::full);
    BOOST_TEST(test_hash_code.length == file_content.size());
    BOOST_TEST(dogbox::count_blobs(*database) >= 1);
    dogbox::tree::regular_file_index const index =
        dogbox::tree::load_regular_file_index(*database, test_hash_code.hash_code);
    BOOST_REQUIRE(dogbox::tree::file_size(index) == file_size);
    std::vector<std::byte> read_content(file_size);
    size_t read = 0;
    while (read < file_size)
    {
        size_t const actual_read_size = std::min(file_size - read, read_size);
        BOOST_REQUIRE(actual_read_size ==
                      dogbox::tree::read_file(
                          index, *database, read, gsl::span<std::byte>(read_content.data() + read, actual_read_size)));
        read += actual_read_size;
    }
    BOOST_TEST(std::equal(read_content.begin(), read_content.end(), file_content.begin(), file_content.end()));
}
