#include "directory.h"
#include "common/byte_literal.h"
#include <algorithm>

namespace dogbox::tree
{
    std::optional<std::tuple<decoded_entry, std::byte const *>> decode_entry(std::byte const *in,
                                                                             std::byte const *const end)
    {
        if (in == end)
        {
            return std::nullopt;
        }
        auto const type = static_cast<unsigned char>(*in++);
        switch (type)
        {
        case static_cast<unsigned char>(entry_type::regular_file):
        case static_cast<unsigned char>(entry_type::directory):
            break;

        default:
            return std::nullopt;
        }
        using namespace dogbox::literals;
        auto *const zero = std::find(in, end, 0_b);
        if (zero == end)
        {
            return std::nullopt;
        }
        std::string_view const name(reinterpret_cast<char const *>(in), static_cast<size_t>(std::distance(in, zero)));
        in = (zero + 1);
        sha256_hash_code hash_code;
        if (hash_code.digits.size() > static_cast<size_t>(std::distance(in, end)))
        {
            return std::nullopt;
        }
        std::copy_n(in, hash_code.digits.size(), hash_code.digits.begin());
        in += hash_code.digits.size();
        return std::tuple<decoded_entry, std::byte const *>(
            decoded_entry{static_cast<entry_type>(type), name, hash_code}, in);
    }
}
