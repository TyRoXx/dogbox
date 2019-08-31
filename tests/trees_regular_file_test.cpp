#include "common/byte_literal.h"
#include "trees/regular_file.h"
#include <algorithm>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(regular_file_encode_piece)
{
    std::vector<std::byte> encoded;
    using namespace dogbox::literals;
    std::array<std::byte, 32> const expected = {0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b,
                                                0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b,
                                                0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b};
    dogbox::regular_file::encode_piece(dogbox::sha256_hash_code(), std::back_inserter(encoded));
    BOOST_TEST(std::equal(expected.begin(), expected.end(), encoded.begin(), encoded.end()));
}

BOOST_AUTO_TEST_CASE(regular_file_finish_encoding)
{
    std::vector<std::byte> encoded;
    using namespace dogbox::literals;
    std::array<std::byte, 8> const expected = {0x12_b, 0x34_b, 0x56_b, 0x78_b, 0xaa_b, 0xbb_b, 0xcc_b, 0xdd_b};
    dogbox::regular_file::finish_encoding(expected.data(), expected.size(), std::back_inserter(encoded));
    BOOST_TEST(std::equal(expected.begin(), expected.end(), encoded.begin(), encoded.end()));
}

BOOST_AUTO_TEST_CASE(regular_file_decode_piece_success)
{
    using namespace dogbox::literals;
    std::array<std::byte, 33> const input = {0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b,
                                             0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b,
                                             0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 99_b};
    auto *const end = input.data() + input.size();
    auto const result = dogbox::regular_file::decode_piece(input.data(), end);
    BOOST_REQUIRE(result);
    dogbox::sha256_hash_code const expected{};
    BOOST_TEST(expected == std::get<0>(*result));
    BOOST_TEST((input.data() + expected.digits.size()) == std::get<1>(*result));
}

BOOST_AUTO_TEST_CASE(regular_file_decode_piece_failure)
{
    using namespace dogbox::literals;
    std::array<std::byte, 31> const input = {0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b,
                                             0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b,
                                             0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b, 0_b};
    auto *const end = input.data() + input.size();
    auto const result = dogbox::regular_file::decode_piece(input.data(), end);
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(regular_file_finish_decoding_success)
{
    using namespace dogbox::literals;
    std::array<std::byte, 9> const input = {0x12_b, 0x34_b, 0x56_b, 0x78_b, 0xaa_b, 0xbb_b, 0xcc_b, 0xdd_b, 99_b};
    auto *const end = input.data() + input.size();
    auto *const result = dogbox::regular_file::finish_decoding(input.data(), end, 8);
    BOOST_REQUIRE(result);
    BOOST_TEST(input.data() == result);
}

BOOST_AUTO_TEST_CASE(regular_file_finish_decoding_failure)
{
    using namespace dogbox::literals;
    std::array<std::byte, 9> const input = {0x12_b, 0x34_b, 0x56_b, 0x78_b, 0xaa_b, 0xbb_b, 0xcc_b, 0xdd_b, 99_b};
    auto *const end = input.data() + input.size();
    auto *const result = dogbox::regular_file::finish_decoding(input.data(), end, 10);
    BOOST_REQUIRE(!result);
}
