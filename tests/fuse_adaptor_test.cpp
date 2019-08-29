#include "common/create_random_directory.h"
#include "fuse_adaptor/fuse_adaptor.h"
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

    struct fuse_exiter
    {
        struct fuse &handle;

        ~fuse_exiter() noexcept
        {
            fuse_exit(&handle);
        }
    };

    void unmount(std::filesystem::path const &mount_point)
    {
        boost::process::ipstream standard_error;
        if (boost::process::system(boost::process::search_path("fusermount"), "-zu", mount_point.string(),
                                   boost::process::std_out > stdout, boost::process::std_err > standard_error,
                                   boost::process::std_in < stdin) != 0)
        {
            std::string output;
            std::getline(standard_error, output);
            BOOST_REQUIRE(output == "/bin/fusermount: entry for " + mount_point.string() + " not found in /etc/mtab");
        }
    }

    struct auto_unmounter
    {
        std::filesystem::path mount_point;

        ~auto_unmounter() noexcept
        {
            unmount(mount_point);
        }
    };

    void test_fuse_adaptor(std::filesystem::path const &input_directory)
    {
        fuse_args arguments = {};
        std::filesystem::path const mount_point = "/tmp/dogbox_test_fuse_mount";
        unmount(mount_point);
        std::filesystem::create_directories(mount_point);

        // fuse_unmount has to be called before fuse_destroy. That's why the destruction order of these handles is
        // important.
        std::unique_ptr<struct fuse, dogbox::fuse::fuse_deleter> fuse_handle;

        dogbox::fuse::channel const channel(
            [&]() -> fuse_chan & {
                fuse_chan *const result = fuse_mount(mount_point.c_str(), &arguments);
                BOOST_REQUIRE(result);
                return *result;
            }(),
            mount_point);

        std::future<void> worker;
        {
            // for some reason fuse locks up if we don't explicitly unmount
            auto_unmounter const unmounter{mount_point};

            auto const operations = dogbox::fuse::make_operations();
            dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
            dogbox::initialize_blob_storage(*database);
            dogbox::sha256_hash_code const directory_hash_code = dogbox::import::from_filesystem_directory(
                *database, input_directory, dogbox::import::parallelism::full);
            dogbox::fuse::user_data user_data{*database, directory_hash_code, {}};
            fuse_handle.reset(fuse_new(channel.handle, &arguments, &operations, sizeof(operations), &user_data));
            BOOST_REQUIRE(fuse_handle);

            worker = std::async(std::launch::async, [&]() {
                try
                {
                    BOOST_REQUIRE(0 == fuse_loop(fuse_handle.get()));
                } catch (...)
                {
                    std::terminate();
                }
            });

            {
                fuse_exiter const exiter{*fuse_handle};
                int const exit_code = boost::process::system(
                    boost::process::search_path("diff"), "-r", input_directory.string(), mount_point.string(),
                    boost::process::std_out > stdout, boost::process::std_err > stderr, boost::process::std_in < stdin);
                BOOST_REQUIRE(0 == exit_code);
            }
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
