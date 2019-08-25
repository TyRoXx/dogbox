#include "directory.h"
#include <boost/test/unit_test.hpp>

namespace
{
    std::filesystem::path find_test_directories()
    {
        return std::filesystem::path(__FILE__).parent_path() / "test_directories";
    }

    void test_scan_directory(std::filesystem::path const &relative_path, dogbox::directory const &expected)
    {
        auto const input = find_test_directories() / relative_path;
        auto const scanned = dogbox::scan_directory(input);
        BOOST_TEST(expected == scanned);
    }

    std::vector<std::byte> to_bytes(char const *const c_str)
    {
        return std::vector<std::byte>(reinterpret_cast<std::byte const *>(c_str),
                                      reinterpret_cast<std::byte const *>(c_str) + std::strlen(c_str));
    }
}

BOOST_AUTO_TEST_CASE(scan_directory_empty)
{
    test_scan_directory("empty", dogbox::directory());
}

BOOST_AUTO_TEST_CASE(scan_directory_nested)
{
    using dogbox::directory;
    using dogbox::directory_entry;
    using dogbox::regular_file;
    test_scan_directory(
        "nested",
        directory{
            {{"1", directory_entry{directory{
                       {{"2", directory_entry{directory{{{"in2.txt", directory_entry{regular_file{to_bytes("C")}}}}}}},
                        {"in1.txt", directory_entry{regular_file{to_bytes("B")}}}}}}},
             {"test.txt", directory_entry{regular_file{to_bytes("A")}}}}});
}
