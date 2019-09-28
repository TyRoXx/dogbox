#include "common/create_random_file.h"
#include "common/directory_auto_deleter.h"
#include "common/to_do.h"
#if DOGBOX_WITH_FUSE
#include "in_memory_fuse_frontend_shared/directory.h"
#endif
#include "trees/import.h"
#include <benchmark/benchmark.h>
#include <fstream>
#include <random>

#if DOGBOX_WITH_FUSE
static void benchmark_scan_directory(benchmark::State &state)
{
    for (auto _ : state)
    {
        dogbox::in_memory_fuse::scan_directory(std::filesystem::path(__FILE__).parent_path().parent_path());
    }
}

BENCHMARK(benchmark_scan_directory)->Unit(benchmark::kMillisecond);
#endif

static void benchmark_store_blob(benchmark::State &state)
{
    using random_bytes_engine = std::independent_bits_engine<std::mt19937, 8, unsigned char>;
    random_bytes_engine rbe;
    std::vector<unsigned char> blob(1024 * 1024 * 2);
    std::generate(begin(blob), end(blob), std::ref(rbe));
    for (auto _ : state)
    {
        dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
        dogbox::initialize_blob_storage(*database);
        dogbox::store_blob(*database, reinterpret_cast<std::byte const *>(blob.data()), blob.size());
    }
    state.SetBytesProcessed(state.iterations() * blob.size());
}

BENCHMARK(benchmark_store_blob)->Unit(benchmark::kMillisecond);

static void benchmark_sha256(benchmark::State &state)
{
    using random_bytes_engine = std::independent_bits_engine<std::mt19937, 8, unsigned char>;
    random_bytes_engine rbe;
    std::vector<unsigned char> blob(static_cast<size_t>(state.range(0)));
    std::generate(begin(blob), end(blob), std::ref(rbe));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(dogbox::sha256(reinterpret_cast<std::byte const *>(blob.data()), blob.size()));
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(state.iterations() * blob.size());
}

BENCHMARK(benchmark_sha256)->Unit(benchmark::kMillisecond)->Range(1024 * 1024, 1024 * 1024 * 10);

static void benchmark_import_directory(benchmark::State &state, dogbox::import::parallelism const parallel)
{
    using random_bytes_engine = std::independent_bits_engine<std::mt19937, 8, unsigned char>;
    random_bytes_engine random;
    std::filesystem::path const imported_dir =
        std::filesystem::path("/tmp") / std::to_string(std::uniform_int_distribution<uint64_t>()(random));
    dogbox::directory_auto_deleter const imported_dir_deleter{imported_dir};
    std::filesystem::create_directory(imported_dir);
    size_t const total_file_size = 200 * 1000 * 1000;
    size_t const number_of_files = state.range(0);
    size_t const file_size = (total_file_size / number_of_files);
    for (size_t i = 0; i < number_of_files; ++i)
    {
        dogbox::create_random_file((imported_dir / std::to_string(i)), file_size);
    }
    for (auto _ : state)
    {
        dogbox::sqlite_handle const database = dogbox::open_sqlite(":memory:");
        dogbox::initialize_blob_storage(*database);
        dogbox::import::from_filesystem_directory(*database, imported_dir, parallel);
    }
    state.SetBytesProcessed(state.iterations() * total_file_size);
}

static void benchmark_import_directory_sequential(benchmark::State &state)
{
    benchmark_import_directory(state, dogbox::import::parallelism::none);
}

static void benchmark_import_directory_parallel(benchmark::State &state)
{
    benchmark_import_directory(state, dogbox::import::parallelism::full);
}

BENCHMARK(benchmark_import_directory_sequential)->Unit(benchmark::kMillisecond)->Range(1, 20000);
BENCHMARK(benchmark_import_directory_parallel)->Unit(benchmark::kMillisecond)->Range(1, 20000);

static void benchmark_create_random_file(benchmark::State &state)
{
    uint64_t const size = state.range(0);
    std::filesystem::path const file = "/tmp/dogbox_benchmark_random_file";
    for (auto _ : state)
    {
        dogbox::create_random_file(file, size);
        std::filesystem::remove(file);
    }
    state.SetBytesProcessed(state.iterations() * size);
}

BENCHMARK(benchmark_create_random_file)->Unit(benchmark::kMillisecond)->Range(100 * 1024, 10 * 1024 * 1024);

BENCHMARK_MAIN();
