#include "common/create_random_directory.h"
#include "fuse_write_adaptor/adaptor.h"
#include "trees/import.h"
#include <boost/process/io.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <future>

namespace
{
    std::filesystem::path find_test_directories()
    {
        return std::filesystem::path(__FILE__).parent_path() / "test_directories";
    }

    void require_directories_equal(std::filesystem::path const &first, std::filesystem::path const &second)
    {
        int const exit_code = boost::process::system(boost::process::search_path("diff"), "-r", first.string(),
                                                     second.string(), boost::process::std_out > stdout,
                                                     boost::process::std_err > stderr, boost::process::std_in < stdin);
        BOOST_REQUIRE(0 == exit_code);
    }

    void test_fuse_adaptor(std::filesystem::path const &input_directory,
                           dogbox::tree::read_caching const read_caching_mode)
    {
        std::filesystem::path const mount_point = "/tmp/dogbox_test_fuse_mount";
        dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
        dogbox::initialize_blob_storage(*database);
        dogbox::sha256_hash_code const directory_hash_code =
            dogbox::import::from_filesystem_directory(*database, input_directory, dogbox::import::parallelism::full);
        std::future<void> worker;
        {
            dogbox::fuse::write::adaptor fuse(mount_point, *database, directory_hash_code, read_caching_mode);
            worker = std::async(std::launch::async, [&]() {
                try
                {
                    BOOST_TEST(0 == fuse_loop(&fuse.get_fuse_handle()));
                } catch (...)
                {
                    std::terminate();
                }
            });

            require_directories_equal(input_directory, mount_point);

            fuse_exit(&fuse.get_fuse_handle());
        }
        worker.get();
    }
    constexpr std::string_view test_directories[] = {"empty", "nested", "deeply_nested"};
}

BOOST_DATA_TEST_CASE(fuse_write_adaptor_read_small,
                     boost::unit_test::data::make(test_directories) *
                         boost::unit_test::data::make(dogbox::tree::all_read_caching_modes),
                     input_directory, read_caching_mode)
{
    test_fuse_adaptor(find_test_directories() / input_directory, read_caching_mode);
}

BOOST_DATA_TEST_CASE(fuse_write_adaptor_read_large,
                     boost::unit_test::data::make({1, 20}) *
                         boost::unit_test::data::make<dogbox::regular_file::length_type>(
                             {0, dogbox::regular_file::piece_length, dogbox::regular_file::piece_length * 41}) *
                         boost::unit_test::data::make(dogbox::tree::all_read_caching_modes),
                     number_of_files, total_size, read_caching_mode)
{
    std::filesystem::path const directory = "/tmp/dogbox_random_dir";
    dogbox::create_random_directory(directory, number_of_files, total_size);
    test_fuse_adaptor(directory, read_caching_mode);
}

BOOST_AUTO_TEST_CASE(fuse_write_adaptor_remove)
{
    std::filesystem::path const mount_point = "/tmp/dogbox_test_fuse_mount";
    std::filesystem::path const reference_directory = "/tmp/dogbox_reference";
    std::filesystem::remove_all(reference_directory);
    std::filesystem::create_directory(reference_directory);

    std::filesystem::path const removed_file_name = "file.txt";
    {
        std::ofstream file((reference_directory / removed_file_name).string(), std::ios::binary);
        file.flush();
        BOOST_REQUIRE(file);
    }

    dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
    dogbox::initialize_blob_storage(*database);
    dogbox::sha256_hash_code const directory_hash_code =
        dogbox::import::from_filesystem_directory(*database, reference_directory, dogbox::import::parallelism::full);
    std::future<void> worker;
    {
        dogbox::fuse::write::adaptor fuse(
            mount_point, *database, directory_hash_code, dogbox::tree::read_caching::one_piece);
        worker = std::async(std::launch::async, [&]() {
            try
            {
                BOOST_TEST(0 == fuse_loop(&fuse.get_fuse_handle()));
            } catch (...)
            {
                std::terminate();
            }
        });

        require_directories_equal(reference_directory, mount_point);

        std::filesystem::remove(reference_directory / removed_file_name);
        std::filesystem::remove(mount_point / removed_file_name);

        require_directories_equal(reference_directory, mount_point);

        fuse_exit(&fuse.get_fuse_handle());
    }
    worker.get();
}
