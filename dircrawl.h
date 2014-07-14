/* The MIT License (MIT)
 *
 * Copyright (c) 2014 Xirdus
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef DIRCRAWL_H
#define DIRCRAWL_H

#include <string>
#include <memory>
#include <utility>
#include <stack>
#include <sstream>

#if defined(_WIN32) && not defined (DIRCRAWL_USE_POSIX) // use Windows API
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else // use POSIX API
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace dirc {

enum class EntryType
{
    unknown,
    file,
    directory
};

enum class CrawlMode
{
    flat_file,
    flat_directory,
    recursive_file
};

#if defined(_WIN32) && not defined (DIRCRAWL_USE_POSIX) // use Windows API

namespace platform {

char separator = '\\';

struct DirHandleWrapper
{
    std::string path;
    HANDLE handle;
};
typedef DirHandleWrapper* DirHandle;

inline DirHandle openHandle(const std::string& path)
{
    return new DirHandleWrapper({path + "\\*", nullptr});
}

inline void closeHandle(DirHandle handle)
{
    if (handle->handle != nullptr && handle->handle != INVALID_HANDLE_VALUE)
    {
        FindClose(handle->handle);
    }
    delete handle;
}

inline std::string getNextItem(DirHandle handle)
{
    WIN32_FIND_DATAA data;
    while (true)
    {

        if (handle->handle == nullptr)
        {
            handle->handle = FindFirstFileA(handle->path.c_str(), &data);
            if (handle->handle == INVALID_HANDLE_VALUE)
            {
                return {};
            }
        }
        else
        {
            if (!FindNextFileA(handle->handle, &data))
            {
                return {};
            }
        }
        std::string name = data.cFileName;
        if (name != "." && name != "..")
        {
            return name;
        }
    }
}

inline EntryType getType(const std::string& path)
{
    auto type = GetFileAttributesA(path.c_str());
    if (type & FILE_ATTRIBUTE_DIRECTORY)
    {
        return EntryType::directory;
    }
    else
    {
        return EntryType::file;
    }
}

}

#else // use POSIX API

namespace platform {

typedef DIR* DirHandle;
char separator = '/';

inline DirHandle openHandle(const std::string& path)
{
    return opendir(path.c_str());
}

inline void closeHandle(DirHandle handle)
{
    closedir(handle);
}

inline std::string getNextItem(DirHandle handle)
{
    while (true)
    {
        auto item = readdir(handle);
        if (item == nullptr)
        {
            return {};
        }
        std::string name = item->d_name;
        if (name != "." && name != "..")
        {
            return name;
        }
    }
}

inline EntryType getType(const std::string& path)
{
    struct stat statinfo;
    stat(path.c_str(), &statinfo);
    if (S_ISREG(statinfo.st_mode))
    {
        return EntryType::file;
    }
    else if (S_ISDIR(statinfo.st_mode))
    {
        return EntryType::directory;
    }
    else
    {
        return EntryType::unknown;
    }
}

}
#endif

class CrawlerIterator: public std::iterator<std::input_iterator_tag, const std::string>
{
public:
    CrawlerIterator() = default;
    CrawlerIterator(const CrawlerIterator&) = default;
    CrawlerIterator(CrawlerIterator&&) = default;
    CrawlerIterator& operator=(const CrawlerIterator&) = default;
    CrawlerIterator& operator=(CrawlerIterator&&) = default;

    CrawlerIterator(const std::string& dir_path, CrawlMode mode):
        dir_paths({dir_path}), mode(mode)
    {
        dir_handles.emplace(platform::openHandle(dir_path), &platform::closeHandle);
        ++*this;
    }

    CrawlerIterator& operator++()
    {
        while (true)
        {
            if (dir_handles.empty())
            {
                return *this;
            }

            while (true)
            {
                item_path = platform::getNextItem(dir_handles.top().get());
                if (item_path.empty())
                {
                    dir_paths.pop_back();
                    dir_handles.pop();
                    break;
                }

                auto type = platform::getType(buildPath() + item_path);
                if (mode != CrawlMode::flat_directory && type == EntryType::file)
                {
                    return *this;
                }
                else if (type == EntryType::directory)
                {
                    if (mode == CrawlMode::flat_directory)
                    {
                        return *this;
                    }
                    else if (mode == CrawlMode::recursive_file)
                    {
                        dir_handles.emplace(platform::openHandle(buildPath() + item_path), &platform::closeHandle);
                        dir_paths.emplace_back(item_path);
                    }

                }
            }
        }
    }

    CrawlerIterator operator++(int)
    {
        CrawlerIterator tmp = *this;
        ++*this;
        return tmp;
    }

    reference operator*() const
    {
        return item_path;
    }

    pointer operator->() const
    {
        return &item_path;
    }

    bool operator==(const CrawlerIterator& rhs)
    {
        return (dir_handles == rhs.dir_handles && item_path == rhs.item_path);
    }

    bool operator!=(const CrawlerIterator& rhs)
    {
        return !(*this == rhs);
    }

private:
    std::string buildPath()
    {
        std::ostringstream path;
        for (auto s: dir_paths)
        {
            path << s << platform::separator;
        }
        return path.str();
    }

    std::deque<std::string> dir_paths;
    CrawlMode mode;
    std::stack<std::shared_ptr<std::remove_pointer<platform::DirHandle>::type>> dir_handles;
    std::string item_path;
};

class DirectoryCrawler
{
public:
    typedef CrawlerIterator iterator;

    DirectoryCrawler() = default;
    DirectoryCrawler(const DirectoryCrawler&) = default;
    DirectoryCrawler(DirectoryCrawler&&) = default;
    DirectoryCrawler& operator=(const DirectoryCrawler&) = default;
    DirectoryCrawler& operator=(DirectoryCrawler&&) = default;

    DirectoryCrawler(const std::string& path, CrawlMode mode):
        path(path), mode(mode) {}

    CrawlerIterator begin()
    {
        return {path, mode};
    }

    CrawlerIterator end()
    {
        return {};
    }

private:
    std::string path;
    CrawlMode mode;
};

inline DirectoryCrawler listFiles(const std::string& path)
{
    return {path, CrawlMode::flat_file};
}

inline DirectoryCrawler listDirectories(const std::string& path)
{
    return {path, CrawlMode::flat_directory};
}

inline DirectoryCrawler listFilesRecursive(const std::string& path)
{
    return {path, CrawlMode::recursive_file};
}

}

#endif // DIRCRAWL_H
