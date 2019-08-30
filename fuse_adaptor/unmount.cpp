#include "unmount.h"
#include "common/to_do.h"
#include <boost/process/io.hpp>
#include <boost/process/search_path.hpp>
#include <boost/process/system.hpp>

namespace dogbox::fuse
{
    void unmount(std::filesystem::path const &mount_point)
    {
        boost::process::ipstream standard_error;
        if (boost::process::system(boost::process::search_path("fusermount"), "-zu", mount_point.string(),
                                   boost::process::std_out > stdout, boost::process::std_err > standard_error,
                                   boost::process::std_in < stdin) != 0)
        {
            std::string output;
            std::getline(standard_error, output);
            if (output != "/bin/fusermount: entry for " + mount_point.string() + " not found in /etc/mtab")
            {
                TO_DO();
            }
        }
    }
}
