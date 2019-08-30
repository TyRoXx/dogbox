#include "hash_code.h"
#include <openssl/sha.h>

namespace dogbox
{
    sha256_hash_code sha256(std::byte const *const data, size_t const size)
    {
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data, size);
        sha256_hash_code result;
        SHA256_Final(reinterpret_cast<unsigned char *>(result.digits.data()), &sha256);
        return result;
    }

    std::string to_string(sha256_hash_code const &hash_code)
    {
        std::string result;
        format_bytes(hash_code.digits, std::back_inserter(result));
        return result;
    }
}
