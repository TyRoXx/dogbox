#include "directory.h"
#include "common/to_do.h"
#include <fstream>

namespace dogbox::in_memory_fuse
{
    std::ostream &operator<<(std::ostream &out, directory const &printed)
    {
        out << "Directory of " << printed.entries.size() << " entries: [\n";
        for (auto const &entry : printed.entries)
        {
            out << entry.first << " = " << entry.second << '\n';
        }
        return out << "]";
    }

    std::ostream &operator<<(std::ostream &out, directory_entry const &printed)
    {
        std::visit(overloaded{[&](regular_file const &regular) { out << regular; },
                              [&](directory const &subdirectory) { out << subdirectory; }},
                   printed.content);
        return out;
    }

    regular_file scan_regular_file(std::filesystem::path const &input)
    {
        std::ifstream file(input.string(), std::ios::binary);
        if (!file)
        {
            TO_DO();
        }
        auto const at_beginning = file.tellg();
        file.seekg(0, std::ios::end);
        auto const at_end = file.tellg();
        file.seekg(0, std::ios::beg);
        auto const size = (at_end - at_beginning);
        std::vector<std::byte> content(static_cast<size_t>(size));
        file.read(reinterpret_cast<char *>(content.data()), size);
        if (!file)
        {
            TO_DO();
        }
        return regular_file{std::move(content)};
    }

    directory scan_directory(std::filesystem::path const &input)
    {
        std::map<std::string, directory_entry> entries;
        std::filesystem::directory_iterator i(input);
        for (; i != std::filesystem::directory_iterator(); ++i)
        {
            auto entry_path = i->path();
            auto entry_name = entry_path.filename();
            switch (i->status().type())
            {
            case std::filesystem::file_type::regular:
                entries.insert(std::make_pair(entry_name, directory_entry{{scan_regular_file(entry_path)}}));
                break;

            case std::filesystem::file_type::directory:
                entries.insert(std::make_pair(entry_name, directory_entry{{scan_directory(entry_path)}}));
                break;

            case std::filesystem::file_type::none:
            case std::filesystem::file_type::not_found:
            case std::filesystem::file_type::symlink:
            case std::filesystem::file_type::block:
            case std::filesystem::file_type::character:
            case std::filesystem::file_type::fifo:
            case std::filesystem::file_type::socket:
            case std::filesystem::file_type::unknown:
                break;
            }
        }
        return directory{std::move(entries)};
    }
}
