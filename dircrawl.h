/* The MIT License (MIT)
 *
 * Copyright (c) 2014 Xirdus
 *
 * Original source: https://github.com/Xirdus/dircrawl
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

#if defined(_WIN32) && not defined (DIRCRAWL_USE_POSIX)
#include "dircrawl_winapi.h"
#else
#include "dircrawl_posix.h"
#endif

namespace dirc {

class CrawlerIterator: public std::iterator<std::input_iterator_tag, const std::string>
{
public:
    CrawlerIterator() = default;
    CrawlerIterator(const CrawlerIterator&) = default;
    CrawlerIterator(CrawlerIterator&&) = default;
    CrawlerIterator& operator=(const CrawlerIterator&) = default;
    CrawlerIterator& operator=(CrawlerIterator&&) = default;

    CrawlerIterator(const std::string& path, CrawlMode mode):
        base_path(path + platform::separator), mode(mode)
    {
        dir_handles.emplace(platform::getHandle(base_path));
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
                item_path = platform::getNextItem(dir_handles.top());
                if (item_path.empty())
                {
                    if (!dir_names.empty())
                    {
                        dir_names.pop_back();
                    }
                    dir_handles.pop();
                    break;
                }

                auto type = platform::getType(base_path + buildPath() + item_path);
                if (mode != CrawlMode::flat_file && type == EntryType::file)
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
                        dir_handles.emplace(platform::getHandle(base_path + buildPath() + item_path));
                        dir_names.emplace_back(item_path);
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
        value = buildPath('/') + item_path;
        return value;
    }

    pointer operator->() const
    {
        value = buildPath('/') + item_path;
        return &value;
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
    std::string buildPath(char separator = platform::separator) const
    {
        std::ostringstream path;
        for (auto s: dir_names)
        {
            path << s << separator;
        }
        return path.str();
    }

    std::string base_path;
    CrawlMode mode;
    std::deque<std::string> dir_names;
    std::stack<platform::DirHandle> dir_handles;
    std::string item_path;
    mutable std::string value;
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
