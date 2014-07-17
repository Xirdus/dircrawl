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

#ifndef DIRCRAWL_POSIX_H
#define DIRCRAWL_POSIX_H

#include <string>
#include <memory>
#include <dirent.h>
#include <sys/stat.h>

#include "dircrawl_types.h"

namespace dirc {
namespace platform {

typedef std::shared_ptr<DIR> DirHandle;
char separator = '/';

inline DirHandle getHandle(const std::string& path)
{
    return std::shared_ptr<DIR>(opendir(path.c_str()), &closedir);
}

inline std::string getNextItem(const DirHandle& handle)
{
    if (!handle)
    {
        return {};
    }

    while (true)
    {
        auto item = readdir(handle.get());
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
}

#endif // DIRCRAWL_POSIX_H
