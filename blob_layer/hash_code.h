#pragma once

#include <array>
#include <optional>
#include <ostream>

namespace dogbox
{
    template <class CharOutputIterator>
    void byte_to_hex(std::byte const in, CharOutputIterator &&out)
    {
        char const *const digits = "0123456789abcdef";
        *out++ = digits[static_cast<unsigned>(in) >> 4];
        *out++ = digits[static_cast<unsigned>(in) & 0x0f];
    }

    template <class ByteInputRange, class CharOutputIterator>
    void format_bytes(ByteInputRange const &input, CharOutputIterator &&output)
    {
        for (std::byte const b : input)
        {
            byte_to_hex(b, output);
        }
    }

    struct sha256_hash_code
    {
        std::array<std::byte, 256 / 8> digits;
    };

    inline bool operator==(sha256_hash_code const &left, sha256_hash_code const &right) noexcept
    {
        return left.digits == right.digits;
    }

    inline std::ostream &operator<<(std::ostream &out, sha256_hash_code const &printed)
    {
        format_bytes(printed.digits, std::ostreambuf_iterator<char>(out));
        return out;
    }

    sha256_hash_code sha256(std::byte const *const data, size_t const size);
    std::string to_string(sha256_hash_code const &hash_code);

    constexpr std::optional<unsigned> decode_hex_digit(char const c)
    {
        switch (c)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return (c - '0');

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            return (c - 'a' + 10);

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            return (c - 'A' + 10);

        default:
            return std::nullopt;
        }
    }

    constexpr std::optional<sha256_hash_code> parse_sha256_hash_code(std::string_view const input)
    {
        sha256_hash_code result = {};
        if (input.size() != (result.digits.size() * 2))
        {
            return std::nullopt;
        }
        for (size_t i = 0; i < result.digits.size(); ++i)
        {
            auto const high = decode_hex_digit(input[i * 2]);
            if (!high)
            {
                return std::nullopt;
            }
            auto const low = decode_hex_digit(input[i * 2 + 1]);
            if (!low)
            {
                return std::nullopt;
            }
            result.digits[i] = static_cast<std::byte>((*high << 4u) | *low);
        }
        return result;
    }
}
