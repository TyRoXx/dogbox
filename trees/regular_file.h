#pragma once

#include "blob_layer/hash_code.h"
#include <algorithm>
#include <cassert>
#include <optional>

namespace dogbox::regular_file
{
    using length_type = uint64_t;

    constexpr length_type piece_length = 1024 * 1024;

    template <class ByteOutputIterator>
    ByteOutputIterator encode_big_endian_integer(uint64_t const value, ByteOutputIterator out)
    {
        for (size_t i = 0; i < sizeof(value); ++i)
        {
            *out++ = static_cast<std::byte>(value >> ((7u - i) * 8u));
        }
        return out;
    }

    template <class Unsigned>
    std::optional<std::tuple<Unsigned, std::byte const *>> decode_big_endian_integer(std::byte const *in,
                                                                                     std::byte const *end)
    {
        size_t const input_size = static_cast<size_t>(std::distance(in, end));
        if (sizeof(Unsigned) > input_size)
        {
            return std::nullopt;
        }
        Unsigned result = 0;
        for (size_t i = 0; i < sizeof(result); ++i, ++in)
        {
            result <<= 8u;
            result |= static_cast<uint64_t>(*in);
        }
        return std::tuple<Unsigned, std::byte const *>(result, in);
    }

    template <class ByteOutputIterator>
    ByteOutputIterator start_encoding(length_type const length, ByteOutputIterator out)
    {
        return encode_big_endian_integer(length, out);
    }

    template <class ByteOutputIterator>
    ByteOutputIterator encode_piece(sha256_hash_code const &hash_code, ByteOutputIterator out)
    {
        return std::copy(hash_code.digits.begin(), hash_code.digits.end(), out);
    }

    template <class ByteOutputIterator>
    ByteOutputIterator finish_encoding(std::byte const *const final_piece, size_t const size, ByteOutputIterator out)
    {
        assert(size < piece_length);
        return std::copy_n(final_piece, size, out);
    }

    std::optional<std::tuple<length_type, std::byte const *>> start_decoding(std::byte const *in, std::byte const *end);
    std::optional<std::tuple<sha256_hash_code, std::byte const *>> decode_piece(std::byte const *in,
                                                                                std::byte const *end);
    std::byte const *finish_decoding(std::byte const *in, std::byte const *end,
                                     length_type const expected_final_piece_length);
}
