#include "blob_storage.h"
#include <cassert>
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
        if (sqlite3_exec(&database, "CREATE TABLE `blob` (hash_code, content, PRIMARY KEY(hash_code))", nullptr,
                         nullptr, &error) == SQLITE_OK)
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
        sqlite_statement_handle const statement =
            prepare(database, "INSERT INTO `blob` (hash_code, content) VALUES (?, ?)");
        std::string const hash_code_string = to_string(hash_code);
        handle_sqlite_error(database, sqlite3_bind_text(statement.get(), 1, hash_code_string.c_str(),
                                                        static_cast<int>(hash_code_string.size()), nullptr));
        handle_sqlite_error(database, sqlite3_bind_blob64(statement.get(), 2, data, size, nullptr));
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
        sqlite_statement_handle const statement = prepare(database, "SELECT content FROM `blob` WHERE hash_code=?");
        std::string const hash_code_string = to_string(hash_code);
        handle_sqlite_error(database, sqlite3_bind_text(statement.get(), 1, hash_code_string.c_str(),
                                                        static_cast<int>(hash_code_string.size()), nullptr));
        int const return_code = sqlite3_step(statement.get());
        switch (return_code)
        {
        case SQLITE_ROW:
        {
            void const *const data = sqlite3_column_blob(statement.get(), 0);
            size_t const size = static_cast<size_t>(sqlite3_column_bytes(statement.get(), 0));
            content.assign(static_cast<std::byte const *>(data), static_cast<std::byte const *>(data) + size);
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
