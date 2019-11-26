#pragma once

#include <cctype>
#include <filesystem>
#include <istream>
#include <regex>
#include <sstream>
#include <vector>

namespace ftr
{
    namespace fs = std::experimental::filesystem::v1;

    std::regex filter2regexp (const std::string &filter)
    {
        /* convert filesystem wildcard (e.g. "image_*_train.jpg") into regexp
        ** change * symbols to .* (any substring)
        ** change . symbols to [.] (exactly . symbol) */
        std::string regexp(filter);
        size_t len = regexp.length();
        for (size_t i = 0; i < len; ++i)
        {
            if (regexp[i] == '*')
            {
                regexp.insert(i, 1, '.');
                ++i;
                ++len;
            }
            else if (regexp[i] == '.')
            {
                regexp.replace(i, 1, "[.]");
                i += 2;
                len += 2;
            }
        }

        return std::regex(regexp);
    };

    void to_lower(std::string & str)
    {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    void read_file_line(std::istream & is, std::vector<std::string> & results, const char separator = ' ')
    {
        results.clear();

        std::string line;
        std::getline(is, line);
        std::stringstream ss(line);
        std::string result;

        while (!ss.eof())
        {
            std::getline(ss, result, separator);
            if (ss.fail())
                break;

            if (!result.empty())
                results.push_back(result);
        }
    }

    inline bool copy_file(const fs::path & file, const fs::path & newfile)
    {
        return fs::copy_file(file, newfile);
    }

    inline void copy_dir(const fs::path & file, const fs::path & newfile, bool recursive)
    {
        recursive ?
            fs::copy(file, newfile, fs::copy_options::recursive) :
            fs::copy(file, newfile);
    }

    inline bool create_dir(const fs::path & dirname)
    {
        return fs::create_directories(dirname);
    }

    inline bool remove_file(const fs::path & file)
    {
        return fs::remove(file);
    }

    inline void remove_dir(const fs::path & dir)
    {
        fs::remove_all(dir);
    }

    fs::path subdirs(const fs::path & path, const fs::path & base_dir)
    {
        return path.string().substr(base_dir.string().length());
    }
}