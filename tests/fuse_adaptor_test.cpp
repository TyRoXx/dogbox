#include "common/create_random_directory.h"
#include "fuse_adaptor/adaptor.h"
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

    void test_fuse_adaptor(std::filesystem::path const &input_directory)
    {
        std::filesystem::path const mount_point = "/tmp/dogbox_test_fuse_mount";
        dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
        dogbox::initialize_blob_storage(*database);
        dogbox::sha256_hash_code const directory_hash_code =
            dogbox::import::from_filesystem_directory(*database, input_directory, dogbox::import::parallelism::full);
        std::future<void> worker;
        {
            dogbox::fuse::adaptor fuse(mount_point, *database, directory_hash_code);
            worker = std::async(std::launch::async, [&]() {
                try
                {
                    BOOST_REQUIRE(0 == fuse_loop(&fuse.get_fuse_handle()));
                } catch (...)
                {
                    std::terminate();
                }
            });

            int const exit_code = boost::process::system(
                boost::process::search_path("diff"), "-r", input_directory.string(), mount_point.string(),
                boost::process::std_out > stdout, boost::process::std_err > stderr, boost::process::std_in < stdin);
            BOOST_REQUIRE(0 == exit_code);
        }
        worker.get();
    }
}

BOOST_AUTO_TEST_CASE(fuse_adaptor_empty)
{
    test_fuse_adaptor(find_test_directories() / "empty");
}

BOOST_AUTO_TEST_CASE(fuse_adaptor_nested)
{
    test_fuse_adaptor(find_test_directories() / "nested");
}

BOOST_AUTO_TEST_CASE(fuse_adaptor_deeply_nested)
{
    test_fuse_adaptor(find_test_directories() / "deeply_nested");
}

BOOST_DATA_TEST_CASE(fuse_adaptor_large,
                     boost::unit_test::data::make({1, 20}) *
                         boost::unit_test::data::make<dogbox::regular_file::length_type>(
                             {0, dogbox::regular_file::piece_length, dogbox::regular_file::piece_length * 41}),
                     number_of_files, total_size)
{
    std::filesystem::path const directory = "/tmp/dogbox_random_dir";
    dogbox::create_random_directory(directory, number_of_files, total_size);
    test_fuse_adaptor(directory);
}
