#include "common/create_random_directory.h"
#include "fuse_read_only_adaptor/adaptor.h"
#include "trees/import.h"
#include <benchmark/benchmark.h>
#include <boost/process/io.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>
#include <future>

namespace
{
    void benchmark_fuse_read_only_adaptor(benchmark::State &state, dogbox::tree::read_caching const read_cache_mode)
    {
        std::filesystem::path const input_directory = "/tmp/dogbox_random_dir";
        size_t const number_of_files = 1;
        auto const total_size = state.range(0) * number_of_files;
        dogbox::create_random_directory(input_directory, number_of_files, total_size);

        std::filesystem::path const mount_point = "/tmp/dogbox_test_fuse_mount";
        dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
        dogbox::initialize_blob_storage(*database);
        dogbox::sha256_hash_code const directory_hash_code =
            dogbox::import::from_filesystem_directory(*database, input_directory, dogbox::import::parallelism::full);

        std::future<void> worker;
        {
            dogbox::fuse::read_only::adaptor fuse(mount_point, *database, directory_hash_code, read_cache_mode);
            worker = std::async(std::launch::async, [&]() {
                try
                {
                    if (0 != fuse_loop(&fuse.get_fuse_handle()))
                    {
                        TO_DO();
                    }
                } catch (...)
                {
                    std::terminate();
                }
            });

            int64_t bytes_processed = 0;
            for (auto _ : state)
            {
                int const exit_code = boost::process::system(
                    boost::process::search_path("diff"), "-r", input_directory.string(), mount_point.string(),
                    boost::process::std_out > stdout, boost::process::std_err > stderr, boost::process::std_in < stdin);
                if (0 != exit_code)
                {
                    TO_DO();
                }
                bytes_processed += total_size;
            }
            state.SetBytesProcessed(bytes_processed);

            fuse_exit(&fuse.get_fuse_handle());
        }
        worker.get();
    }

    void benchmark_fuse_read_only_adaptor_none(benchmark::State &state)
    {
        benchmark_fuse_read_only_adaptor(state, dogbox::tree::read_caching::none);
    }

    void benchmark_fuse_read_only_adaptor_one_piece(benchmark::State &state)
    {
        benchmark_fuse_read_only_adaptor(state, dogbox::tree::read_caching::one_piece);
    }
}

BENCHMARK(benchmark_fuse_read_only_adaptor_none)
    ->Unit(benchmark::kMillisecond)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Range(50 * dogbox::regular_file::piece_length, 375 * dogbox::regular_file::piece_length);

BENCHMARK(benchmark_fuse_read_only_adaptor_one_piece)
    ->Unit(benchmark::kMillisecond)
    ->MeasureProcessCPUTime()
    ->UseRealTime()
    ->Range(50 * dogbox::regular_file::piece_length, 375 * dogbox::regular_file::piece_length);
