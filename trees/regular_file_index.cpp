#include "regular_file_index.h"
#include "common/to_do.h"
#include "regular_file.h"

namespace dogbox::tree
{
    regular_file_index load_regular_file_index(sqlite3 &database, regular_file::length_type const content_size,
                                               blob_hash_code const hash_code)
    {
        if (content_size == 0)
        {
            return regular_file_index{{}, {}};
        }
        std::optional<std::vector<std::byte>> const maybe_loaded = load_blob(database, hash_code);
        if (!maybe_loaded)
        {
            TO_DO();
        }
        std::vector<std::byte> const &loaded = *maybe_loaded;
        std::byte const *const begin = loaded.data();
        std::byte const *const end = begin + loaded.size();
        std::vector<blob_hash_code> pieces(content_size / regular_file::piece_length);
        std::byte const *cursor = begin;
        for (size_t i = 0; i < pieces.size(); ++i)
        {
            std::optional<std::tuple<sha256_hash_code, std::byte const *>> const piece =
                regular_file::decode_piece(cursor, end);
            if (!piece)
            {
                TO_DO();
            }
            pieces[i] = std::get<0>(*piece);
            cursor = std::get<1>(*piece);
        }
        regular_file::length_type const tail_size = (content_size - (pieces.size() * regular_file::piece_length));
        std::byte const *const tail = regular_file::finish_decoding(cursor, end, tail_size);
        if (!tail)
        {
            TO_DO();
        }
        return tree::regular_file_index{std::move(pieces), std::vector<std::byte>(tail, tail + tail_size)};
    }
}
