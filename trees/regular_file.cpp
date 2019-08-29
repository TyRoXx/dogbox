#include "regular_file.h"
#include <tuple>

namespace dogbox::regular_file
{
    std::optional<std::tuple<length_type, std::byte const *>> start_decoding(std::byte const *in, std::byte const *end)
    {
        return decode_big_endian_integer<length_type>(in, end);
    }

    std::optional<std::tuple<sha256_hash_code, std::byte const *>> decode_piece(std::byte const *in,
                                                                                std::byte const *end)
    {
        sha256_hash_code result;
        if (result.digits.size() > static_cast<size_t>(std::distance(in, end)))
        {
            return std::nullopt;
        }
        std::copy_n(in, result.digits.size(), result.digits.begin());
        in += result.digits.size();
        return std::tuple<sha256_hash_code, std::byte const *>(result, in);
    }

    std::byte const *finish_decoding(std::byte const *in, std::byte const *end,
                                     length_type const expected_final_piece_length)
    {
        if (expected_final_piece_length > static_cast<size_t>(std::distance(in, end)))
        {
            return nullptr;
        }
        return in;
    }
}
