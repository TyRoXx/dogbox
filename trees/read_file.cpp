#include "read_file.h"
#include "common/to_do.h"
#include <cstring>

namespace dogbox::tree
{
    size_t read_file(open_file &file, sqlite3 &database, regular_file::length_type const offset,
                     gsl::span<std::byte> const into, read_caching const caching)
    {
        assert(file.index);
        auto &index = *file.index;
        size_t remaining_size = into.size();
        regular_file::length_type read_cursor = static_cast<regular_file::length_type>(offset);
        std::byte *write_cursor = into.data();
        while (remaining_size > 0)
        {
            size_t const current_piece_index = (read_cursor / regular_file::piece_length);
            std::vector<std::byte> const *piece = nullptr;
            std::optional<std::vector<std::byte>> loaded_piece;
            if (current_piece_index == file.cached_piece)
            {
                piece = &file.read_cache;
            }
            else if (current_piece_index < index.pieces.size())
            {
                loaded_piece = load_blob(database, index.pieces[current_piece_index]);
                if (!loaded_piece)
                {
                    TO_DO();
                }
                piece = [&]() {
                    switch (caching)
                    {
                    case read_caching::none:
                        return &*loaded_piece;

                    case read_caching::one_piece:
                        file.read_cache = std::move(*loaded_piece);
                        file.cached_piece = current_piece_index;
                        return &file.read_cache;
                    }
                    TO_DO();
                }();
            }
            else
            {
                piece = &index.tail;
            }
            size_t const offset_in_piece = (read_cursor % regular_file::piece_length);
            if (offset_in_piece >= piece->size())
            {
                break;
            }
            size_t const copying = std::min(piece->size() - offset_in_piece, remaining_size);
            std::memcpy(write_cursor, piece->data() + offset_in_piece, copying);
            write_cursor += copying;
            read_cursor += copying;
            remaining_size -= copying;
        }
        return static_cast<size_t>(std::distance(into.data(), write_cursor));
    }
}
