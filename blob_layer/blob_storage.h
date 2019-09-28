#pragma once

#include "hash_code.h"
#include <cassert>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include <vector>
#include <string>

namespace dogbox
{
    struct sqlite_error : std::runtime_error
    {
        explicit sqlite_error(int error_code, char const *const message)
            : std::runtime_error("SQLite operation failed with: " + std::to_string(error_code) + "; " + message)
        {
        }
    };

    struct sqlite_deleter
    {
        void operator()(sqlite3 *const database) const
        {
            sqlite3_close(database);
        }
    };

    using sqlite_handle = std::unique_ptr<sqlite3, sqlite_deleter>;

    inline sqlite_handle open_sqlite(char const *const name)
    {
        sqlite3 *database = nullptr;
        sqlite3_open(name, &database);
        return sqlite_handle(database);
    }

    using blob_hash_code = sha256_hash_code;

    void initialize_blob_storage(sqlite3 &database);

    blob_hash_code store_blob(sqlite3 &database, std::byte const *const data, size_t const size);

    std::optional<std::vector<std::byte>> load_blob(sqlite3 &database, blob_hash_code const hash_code);

    bool load_blob(sqlite3 &database, blob_hash_code const hash_code, std::vector<std::byte> &content);

    uint64_t count_blobs(sqlite3 &database);
}
