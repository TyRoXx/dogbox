#include "blob_storage.h"
#include "common/to_do.h"
#include <cassert>
#include <lz4.h>
#include <memory>

namespace dogbox
{
    namespace
    {
        struct sqlite_malloc_deleter
        {
            void operator()(void *const memory) const noexcept
            {
                sqlite3_free(memory);
            }
        };

        struct sqlite_statement_deleter
        {
            void operator()(sqlite3_stmt *const statement) const noexcept
            {
                sqlite3_finalize(statement);
            }
        };

        [[noreturn]] void throw_sqlite_error(sqlite3 &database, int const error)
        {
            throw sqlite_error(error, sqlite3_errmsg(&database));
        }

        void handle_sqlite_error(sqlite3 &database, int const error)
        {
            if (error != SQLITE_OK)
            {
                throw_sqlite_error(database, error);
            }
        }

        using sqlite_statement_handle = std::unique_ptr<sqlite3_stmt, sqlite_statement_deleter>;

        sqlite_statement_handle prepare(sqlite3 &database, char const *const sql)
        {
            sqlite3_stmt *statement = nullptr;
            handle_sqlite_error(database, sqlite3_prepare(&database, sql, -1, &statement, nullptr));
            return sqlite_statement_handle(statement);
        }
    }

    void initialize_blob_storage(sqlite3 &database)
    {
        char *error = nullptr;
        if (sqlite3_exec(&database,
                         "CREATE TABLE `blob` (hash_code, compressed_content, original_size, PRIMARY KEY(hash_code))",
                         nullptr, nullptr, &error) == SQLITE_OK)
        {
            return;
        }
        std::unique_ptr<char, sqlite_malloc_deleter> memory(error);
        std::string message = error;
        throw std::runtime_error(move(message));
    }

    blob_hash_code store_blob(sqlite3 &database, std::byte const *const data, size_t const size)
    {
        blob_hash_code const hash_code = sha256(data, size);
        if (load_blob(database, hash_code))
        {
            return hash_code;
        }

        if (size > std::numeric_limits<int>::max())
        {
            TO_DO();
        }
        int const compressed_size_bound = LZ4_compressBound(static_cast<int>(size));
        assert(compressed_size_bound >= 0);
        std::unique_ptr<std::byte[]> compressed(new std::byte[static_cast<size_t>(compressed_size_bound)]);
        int const compressed_size =
            LZ4_compress_default(reinterpret_cast<char const *>(data), reinterpret_cast<char *>(compressed.get()),
                                 static_cast<int>(size), compressed_size_bound);
        assert(compressed_size <= compressed_size_bound);

        sqlite_statement_handle const statement =
            prepare(database, "INSERT INTO `blob` (hash_code, compressed_content, original_size) VALUES (?, ?, ?)");
        std::string const hash_code_string = to_string(hash_code);
        handle_sqlite_error(database, sqlite3_bind_text(statement.get(), 1, hash_code_string.c_str(),
                                                        static_cast<int>(hash_code_string.size()), nullptr));
        handle_sqlite_error(database, sqlite3_bind_blob64(statement.get(), 2, compressed.get(),
                                                          static_cast<size_t>(compressed_size), nullptr));
        handle_sqlite_error(database, sqlite3_bind_int64(statement.get(), 3, static_cast<sqlite3_int64>(size)));
        int const return_code = sqlite3_step(statement.get());
        switch (return_code)
        {
        case SQLITE_DONE:
            return hash_code;

        default:
            throw_sqlite_error(database, return_code);
        }
    }

    std::optional<std::vector<std::byte>> load_blob(sqlite3 &database, blob_hash_code const hash_code)
    {
        std::vector<std::byte> content;
        if (load_blob(database, hash_code, content))
        {
            return std::move(content);
        }
        return std::nullopt;
    }

    bool load_blob(sqlite3 &database, blob_hash_code const hash_code, std::vector<std::byte> &content)
    {
        sqlite_statement_handle const statement =
            prepare(database, "SELECT compressed_content, original_size FROM `blob` WHERE hash_code=?");
        std::string const hash_code_string = to_string(hash_code);
        handle_sqlite_error(database, sqlite3_bind_text(statement.get(), 1, hash_code_string.c_str(),
                                                        static_cast<int>(hash_code_string.size()), nullptr));
        int const return_code = sqlite3_step(statement.get());
        switch (return_code)
        {
        case SQLITE_ROW:
        {
            void const *const data = sqlite3_column_blob(statement.get(), 0);
            size_t const compressed_size = static_cast<size_t>(sqlite3_column_bytes(statement.get(), 0));
            sqlite3_int64 const original_size = sqlite3_column_int64(statement.get(), 1);
            if (original_size < 0)
            {
                TO_DO();
            }
            if (original_size > std::numeric_limits<int>::max())
            {
                TO_DO();
            }
            if (compressed_size > static_cast<size_t>(std::numeric_limits<int>::max()))
            {
                TO_DO();
            }
            content.resize(static_cast<size_t>(original_size));
            LZ4_decompress_safe(static_cast<char const *>(data), reinterpret_cast<char *>(content.data()),
                                static_cast<int>(compressed_size), static_cast<int>(original_size));
            return true;
        }

        case SQLITE_DONE:
            return false;

        default:
            throw_sqlite_error(database, return_code);
        }
    }

    uint64_t count_blobs(sqlite3 &database)
    {
        sqlite_statement_handle const statement = prepare(database, "SELECT COUNT(*) FROM `blob`");
        int const return_code = sqlite3_step(statement.get());
        switch (return_code)
        {
        case SQLITE_ROW:
        {
            sqlite3_int64 const count = sqlite3_column_int64(statement.get(), 0);
            assert(count >= 0);
            return static_cast<uint64_t>(count);
        }

        default:
            throw_sqlite_error(database, return_code);
        }
    }
}
