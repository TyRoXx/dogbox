#include "common/byte_literal.h"
#include "trees/directory.h"
#include <algorithm>
#include <boost/test/unit_test.hpp>

namespace
{
    using namespace dogbox::literals;
    std::byte const dummy_entry[] = {254_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     123_b,
                                     static_cast<std::byte>('t'),
                                     static_cast<std::byte>('e'),
                                     static_cast<std::byte>('s'),
                                     static_cast<std::byte>('t'),
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b,
                                     0_b};
}

BOOST_AUTO_TEST_CASE(trees_encode_entry)
{
    std::vector<std::byte> encoded;
    dogbox::tree::encode_entry(
        dogbox::tree::entry_type::regular_file, "test", dogbox::sha256_hash_code{}, 123, std::back_inserter(encoded));
    BOOST_TEST(std::equal(std::begin(dummy_entry), std::end(dummy_entry), encoded.begin(), encoded.end()));
}

BOOST_AUTO_TEST_CASE(trees_decode_entry)
{
    auto *const in = dummy_entry;
    auto *const end = in + std::size(dummy_entry);
    auto const result = dogbox::tree::decode_entry(in, end);
    BOOST_REQUIRE(result);
    BOOST_TEST((dogbox::tree::decoded_entry{dogbox::tree::entry_type::regular_file, "test", dogbox::sha256_hash_code{},
                                            123}) == std::get<0>(*result));
    BOOST_TEST(end == std::get<1>(*result));
}
