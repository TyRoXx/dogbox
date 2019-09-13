#pragma once

#include <filesystem>

namespace dogbox::fuse
{
    void unmount(std::filesystem::path const &mount_point);
}
