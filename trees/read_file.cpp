#include "read_file.h"
#include "common/to_do.h"
#include <cstring>

namespace dogbox::tree
{
    size_t read_file(regular_file_index const &file, sqlite3 &database, regular_file::length_type const offset,
                     gsl::span<std::byte> const into)
    {
        size_t remaining_size = into.size();
        regular_file::length_type read_cursor = static_cast<regular_file::length_type>(offset);
        std::byte *write_cursor = into.data();
        while (remaining_size > 0)
        {
            size_t const current_piece_index = (read_cursor / regular_file::piece_length);
            std::vector<std::byte> const *piece = nullptr;
            std::optional<std::vector<std::byte>> loaded_piece;
            if (current_piece_index < file.pieces.size())
            {
                loaded_piece = load_blob(database, file.pieces[current_piece_index]);
                if (!loaded_piece)
                {
                    TO_DO();
                }
                piece = &*loaded_piece;
            }
            else
            {
                piece = &file.tail;
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
