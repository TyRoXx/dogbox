#include "split_path.h"
#include <algorithm>

namespace dogbox
{
    path_split_result split_path(std::filesystem::path const &original)
    {
        auto const &original_string = original.string();
        auto const slash = std::find(original_string.begin(), original_string.end(), '/');
        if (slash == original_string.end())
        {
            return path_split_result{original_string, ""};
        }
        return path_split_result{std::filesystem::path(original_string.begin(), slash),
                                 std::filesystem::path(slash + 1, original_string.end())};
    }
}
