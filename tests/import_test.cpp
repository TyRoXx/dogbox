#include "common/byte_literal.h"
#include "common/create_random_file.h"
#include "common/create_randomness.h"
#include "common/directory_auto_deleter.h"
#include "common/to_do.h"
#include "trees/import.h"
#include "trees/read_file.h"
#include "trees/regular_file.h"
#include "trees/regular_file_index.h"
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <fstream>

namespace
{
    std::filesystem::path find_test_directories()
    {
        return std::filesystem::path(__FILE__).parent_path() / "test_directories";
    }

    constexpr dogbox::import::parallelism all_parallelism_modes[] = {
        dogbox::import::parallelism::none, dogbox::import::parallelism::full};

    enum class regular_file_example
    {
#ifdef _WIN32
#undef small
#endif

        small,
        medium,
        large
    };

    std::ostream &operator<<(std::ostream &out, regular_file_example const printed)
    {
        switch (printed)
        {
        case regular_file_example::small:
            return out << "small";
        case regular_file_example::medium:
            return out << "medium";
        case regular_file_example::large:
            return out << "large";
        }
        TO_DO();
    }

    constexpr regular_file_example all_regular_file_examples[] = {
        regular_file_example::small, regular_file_example::medium, regular_file_example::large};

    std::vector<std::byte> generate_randomness(size_t const size)
    {
        std::vector<std::byte> result(size);
        dogbox::create_randomness(result);
        return result;
    }

    std::vector<std::byte> get_example_file(regular_file_example const type)
    {
        using namespace dogbox::literals;
        switch (type)
        {
        case regular_file_example::small:
            return {static_cast<std::byte>('A')};

        case regular_file_example::medium:
            return generate_randomness(3 * dogbox::regular_file::piece_length + 42);

        case regular_file_example::large:
            return generate_randomness(15 * dogbox::regular_file::piece_length + 42);
        }
        TO_DO();
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

    std::vector<std::byte> load_regular_file_from_database(sqlite3 &database,
                                                           dogbox::regular_file::length_type const content_size,
                                                           dogbox::blob_hash_code const hash_code)
    {
        dogbox::tree::regular_file_index index =
            dogbox::tree::load_regular_file_index(database, content_size, hash_code);
        dogbox::regular_file::length_type const size = dogbox::tree::file_size(index);
        if (size > (std::numeric_limits<size_t>::max)())
        {
            TO_DO();
        }
        std::vector<std::byte> content(static_cast<size_t>(size));
        dogbox::tree::open_file open_file{content_size, hash_code, std::move(index), std::nullopt, {}};
        dogbox::tree::read_file(open_file, database, 0, content, dogbox::tree::read_caching::none);
        return content;
    }
}

BOOST_DATA_TEST_CASE(import_from_filesystem_regular_file, boost::unit_test::data::make(all_parallelism_modes) *
                                                              boost::unit_test::data::make(all_regular_file_examples),
                     parallel, file_example)
{
    dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
    dogbox::initialize_blob_storage(*database);
    BOOST_TEST(dogbox::count_blobs(*database) == 0);
    std::filesystem::path const file = "/tmp/dogbox_regular_file";
    std::vector<std::byte> const file_content = get_example_file(file_example);
    write_file(file, file_content);
    dogbox::import::regular_file_imported const test_hash_code =
        dogbox::import::from_filesystem_regular_file(*database, file, parallel);
    BOOST_TEST(test_hash_code.content_size == file_content.size());
    BOOST_TEST(dogbox::count_blobs(*database) >= 1);
    std::vector<std::byte> const content =
        load_regular_file_from_database(*database, test_hash_code.content_size, test_hash_code.hash_code);
    BOOST_TEST(std::equal(file_content.begin(), file_content.end(), content.begin(), content.end()));
}

namespace
{
    struct test_directory
    {
        std::string_view name;
        dogbox::blob_hash_code root;
        uint64_t blob_count;
    };

    std::ostream &operator<<(std::ostream &out, test_directory const &printed)
    {
        return out << printed.name << " " << printed.root;
    }

    constexpr test_directory test_directories[] = {
        test_directory{
            "empty",
            dogbox::parse_sha256_hash_code("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855").value(),
            1},
        test_directory{
            "nested",
            dogbox::parse_sha256_hash_code("0bd72663b61480486cdf1a985d4fc5adef06f6ca5e42a748bd70eddb8a87bee9").value(),
            6},
        test_directory{
            "deeply_nested",
            dogbox::parse_sha256_hash_code("ef247e30dcf4a7aeb0124f600f5b49d69344505ae41855f918a738d63200b465").value(),
            28}};
}

BOOST_DATA_TEST_CASE(import_from_filesystem_directory_small,
                     boost::unit_test::data::make(test_directories) * all_parallelism_modes, input_directory, parallel)
{
    dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
    dogbox::initialize_blob_storage(*database);
    BOOST_TEST(dogbox::count_blobs(*database) == 0);
    dogbox::sha256_hash_code const directory_hash_code =
        dogbox::import::from_filesystem_directory(*database, find_test_directories() / input_directory.name, parallel);
    BOOST_TEST(input_directory.root == directory_hash_code);
    BOOST_TEST(dogbox::count_blobs(*database) == input_directory.blob_count);
    std::optional<std::vector<std::byte>> const content = dogbox::load_blob(*database, directory_hash_code);
    BOOST_REQUIRE(content);
    // TODO check whether content is correct
}

BOOST_DATA_TEST_CASE(import_from_filesystem_directory_large, all_parallelism_modes, parallel)
{
    using random_bytes_engine = std::independent_bits_engine<std::mt19937, 8, unsigned char>;
    random_bytes_engine random;
    std::filesystem::path const imported_dir =
        std::filesystem::path("/tmp") / std::to_string(std::uniform_int_distribution<uint64_t>()(random));
    dogbox::directory_auto_deleter const imported_dir_deleter{imported_dir};
    std::filesystem::create_directory(imported_dir);
    size_t total_file_size = 0;
    for (size_t i = 0; i < 2; ++i)
    {
        size_t const file_size = dogbox::regular_file::piece_length * 12;
        dogbox::create_random_file((imported_dir / std::to_string(i)), file_size);
        total_file_size += file_size;
    }
    dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
    dogbox::initialize_blob_storage(*database);
    BOOST_TEST(dogbox::count_blobs(*database) == 0);
    dogbox::sha256_hash_code const hash_code =
        dogbox::import::from_filesystem_directory(*database, imported_dir, parallel);
    BOOST_TEST(dogbox::count_blobs(*database) == 27);
    std::optional<std::vector<std::byte>> const content = dogbox::load_blob(*database, hash_code);
    BOOST_REQUIRE(content);
    // TODO check whether content is correct
}
