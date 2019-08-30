#include "adaptor.h"
#include "common/to_do.h"
#include "unmount.h"

namespace dogbox::fuse
{
    fuse_args adaptor::no_arguments = {};

    adaptor::adaptor(std::filesystem::path const &mount_point, sqlite3 &database, dogbox::blob_hash_code const root)
        : user_data{database, root, {}}
        , fuse_handle()
        , channel(
              [&]() -> fuse_chan & {
                  unmount(mount_point);
                  std::filesystem::create_directories(mount_point);
                  fuse_chan *const result = fuse_mount(mount_point.c_str(), &no_arguments);
                  if (!result)
                  {
                      TO_DO();
                  }
                  return *result;
              }(),
              mount_point)
    {
        fuse_handle.reset(fuse_new(channel.handle, &no_arguments, &operations, sizeof(operations), &user_data));
        if (!fuse_handle)
        {
            TO_DO();
        }
    }

    struct fuse &adaptor::get_fuse_handle() const
    {
        return *fuse_handle;
    }
}
