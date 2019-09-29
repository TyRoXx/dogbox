#pragma once

#include "to_do.h"
#include <boost/outcome/result.hpp>
#include <filesystem>

#ifdef __linux
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace dogbox
{
    struct file_descriptor
    {
        int handle;

        file_descriptor() noexcept
            : handle(-1)
        {
        }

        explicit file_descriptor(int handle_)
            : handle(handle_)
        {
        }

        file_descriptor(file_descriptor &&other) noexcept
            : handle(other.handle)
        {
            other.handle = -1;
        }

        file_descriptor &operator=(file_descriptor &&other) noexcept
        {
            std::swap(handle, other.handle);
            return *this;
        }

        ~file_descriptor() noexcept
        {
            if (handle < 0)
            {
                return;
            }
            close(handle);
        }
    };

    inline boost::outcome_v2::result<file_descriptor> open_file_for_reading(std::filesystem::path const &name)
    {
        int const result = open(name.c_str(), O_RDONLY);
        if (result < 0)
        {
            TO_DO();
        }
        return file_descriptor(result);
    }

    inline boost::outcome_v2::result<file_descriptor> create_file(std::filesystem::path const &name)
    {
        int const result = open(name.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0555);
        if (result < 0)
        {
            TO_DO();
        }
        return file_descriptor(result);
    }

    inline void read_at(int const handle, uint64_t const offset, std::byte *into, size_t const count)
    {
        off_t const result = pread64(handle, into, count, offset);
        if (result != static_cast<off_t>(count))
        {
            TO_DO();
        }
    }
} // namespace dogbox
#else
// include Asio before Windows.h to avoid "#error:  WinSock.h has already been included"
#include <Windows.h>
#include <boost/asio/windows/object_handle.hpp>

namespace dogbox
{
    struct file_descriptor
    {
        HANDLE handle;

        file_descriptor() noexcept
            : handle(INVALID_HANDLE_VALUE)
        {
        }

        explicit file_descriptor(HANDLE handle_)
            : handle(handle_)
        {
        }

        file_descriptor(file_descriptor &&other) noexcept
            : handle(other.handle)
        {
            other.handle = INVALID_HANDLE_VALUE;
        }

        file_descriptor &operator=(file_descriptor &&other) noexcept
        {
            std::swap(handle, other.handle);
            return *this;
        }

        ~file_descriptor() noexcept
        {
            if (handle == INVALID_HANDLE_VALUE)
            {
                return;
            }
            CloseHandle(handle);
        }
    };

    inline boost::outcome_v2::result<file_descriptor> open_file_for_reading(std::filesystem::path const &name)
    {
        HANDLE const result = CreateFileW(
            name.native().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (result == INVALID_HANDLE_VALUE)
        {
            TO_DO();
        }
        return file_descriptor(result);
    }

    inline boost::outcome_v2::result<file_descriptor> create_file(std::filesystem::path const &name)
    {
        HANDLE const result =
            CreateFileW(name.native().c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (result == INVALID_HANDLE_VALUE)
        {
            TO_DO();
        }
        return file_descriptor(result);
    }

    inline void read_at(HANDLE const handle, uint64_t const offset, std::byte *into, size_t const count)
    {
        if (count > (std::numeric_limits<DWORD>::max)())
        {
            TO_DO();
        }
        DWORD read = 0;
        OVERLAPPED overlapped = {};
        overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32u);
        overlapped.Offset = static_cast<DWORD>(offset);
        if (!ReadFile(handle, into, static_cast<DWORD>(count), &read, &overlapped))
        {
            TO_DO();
        }
    }
} // namespace dogbox
#endif
