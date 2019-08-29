#pragma once

#include "blob_layer/hash_code.h"
#include "regular_file.h"
#include <optional>
#include <tuple>

namespace dogbox
{
    namespace tree
    {
        enum class entry_type
        {
            regular_file = 254,
            directory = 255
        };

        inline std::ostream &operator<<(std::ostream &out, entry_type const printed)
        {
            switch (printed)
            {
            case entry_type::regular_file:
                return out << "regular_file";
            case entry_type::directory:
                return out << "directory";
            }
            return out << "???";
        }

        template <class ByteOutputIterator>
        void encode_entry(entry_type const type, std::string_view const name, sha256_hash_code const &hash_code,
                          regular_file::length_type const regular_file_size, ByteOutputIterator out)
        {
            *out++ = static_cast<std::byte>(type);
            switch (type)
            {
            case entry_type::directory:
                break;

            case entry_type::regular_file:
                out = regular_file::encode_big_endian_integer(regular_file_size, out);
                break;
            }
            std::byte const *const name_begin = reinterpret_cast<std::byte const *>(name.data());
            out = std::copy(name_begin, name_begin + name.size(), out);
            *out++ = static_cast<std::byte>(0);
            out = std::copy(hash_code.digits.begin(), hash_code.digits.end(), out);
        }

        struct decoded_entry
        {
            entry_type type;
            std::string_view name;
            sha256_hash_code hash_code;
            regular_file::length_type regular_file_size;
        };

        inline bool operator==(decoded_entry const &left, decoded_entry const &right) noexcept
        {
            return std::tie(left.type, left.name, left.hash_code, left.regular_file_size) ==
                   std::tie(right.type, right.name, right.hash_code, right.regular_file_size);
        }

        inline std::ostream &operator<<(std::ostream &out, decoded_entry const &printed)
        {
            return out << printed.type << " " << printed.name << " " << printed.hash_code << " "
                       << printed.regular_file_size;
        }

        std::optional<std::tuple<decoded_entry, std::byte const *>> decode_entry(std::byte const *in,
                                                                                 std::byte const *const end);
    }
}
