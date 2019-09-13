#pragma once

#include <filesystem>
#include <fuse.h>

namespace dogbox::fuse
{
    struct channel
    {
        fuse_chan *handle;
        std::filesystem::path mount_point;

        channel() noexcept
        {
        }

        channel(fuse_chan &handle_, std::filesystem::path mount_point_) noexcept
            : handle(&handle_)
            , mount_point(std::move(mount_point_))
        {
        }

        ~channel() noexcept
        {
            if (!handle)
            {
                return;
            }
            fuse_unmount(mount_point.c_str(), handle);
        }

        channel(channel &&other) noexcept
            : handle(other.handle)
            , mount_point(std::move(other.mount_point))
        {
            other.handle = nullptr;
            other.mount_point = "";
        }

        channel &operator=(channel &&other) noexcept
        {
            using std::swap;
            swap(handle, other.handle);
            swap(mount_point, other.mount_point);
            return *this;
        }
    };
}
