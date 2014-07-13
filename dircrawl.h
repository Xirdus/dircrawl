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

#if defined(_WIN32) && not defined (DIRCRAWL_USE_POSIX) // use Windows API
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

template <EntryType type>
class FlatIterator: std::iterator<std::input_iterator_tag, const std::string>
{
public:
    FlatIterator() = default;
    FlatIterator(const FlatIterator&) = default;
    FlatIterator(FlatIterator&&) = default;
    FlatIterator& operator=(const FlatIterator&) = default;
    FlatIterator& operator=(FlatIterator&&) = default;

    FlatIterator(const std::string& dir_path):
        dir_path(dir_path)
    {
        dir.reset(platform::openHandle(dir_path.c_str()), &platform::closeHandle);
        ++*this;
    }

    FlatIterator& operator++()
    {
        if (!dir)
        {
            return *this;
        }

        do
        {
            path = platform::getNextItem(dir.get());
            if (path.empty())
            {
                dir.reset();
                return *this;
            }
        }
        while (platform::getType(dir_path + platform::separator + path) != type);
        return *this;
    }

    FlatIterator operator++(int)
    {
        FlatIterator tmp = *this;
        ++*this;
        return tmp;
    }

    reference operator*() const
    {
        return path;
    }

    pointer operator->() const
    {
        return &path;
    }

    bool operator==(const FlatIterator& rhs)
    {
        return (!dir && !rhs.dir) || (dir.get() == rhs.dir.get() && path == rhs.path);
    }

    bool operator!=(const FlatIterator& rhs)
    {
        return !(*this == rhs);
    }

private:
    std::shared_ptr<std::remove_pointer<platform::DirHandle>::type> dir;
    std::string dir_path;
    std::string path;
};

template <typename IteratorType>
class DirectoryCrawler
{
public:
    typedef IteratorType iterator;

    DirectoryCrawler() = default;
    DirectoryCrawler(const DirectoryCrawler&) = default;
    DirectoryCrawler(DirectoryCrawler&&) = default;
    DirectoryCrawler& operator=(const DirectoryCrawler&) = default;
    DirectoryCrawler& operator=(DirectoryCrawler&&) = default;

    DirectoryCrawler(const std::string& path):
        path(path) {}

    IteratorType begin()
    {
        return {path};
    }

    IteratorType end()
    {
        return {};
    }

private:
    std::string path;
};

inline DirectoryCrawler<FlatIterator<EntryType::file>> listFiles(const std::string& path)
{
    return {path};
}

inline DirectoryCrawler<FlatIterator<EntryType::directory>> listDirectories(const std::string& path)
{
    return {path};
}

}

#endif // DIRCRAWL_H
