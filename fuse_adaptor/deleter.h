#pragma once

#include <fuse.h>

namespace dogbox::fuse
{
    struct fuse_deleter
    {
        void operator()(struct fuse *const handle) const noexcept
        {
            fuse_destroy(handle);
        }
    };
}
