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

#ifndef DIRCRAWL_WINAPI_H
#define DIRCRAWL_WINAPI_H

#define WIN32_LEAN_AND_MEAN
#include <string>
#include <memory>
#include <windows.h>

#include "dircrawl_types.h"

namespace dirc {
namespace platform {

char separator = '\\';

struct DirHandleWrapper
{
    std::string path;
    HANDLE handle;
};
typedef std::shared_ptr<DirHandleWrapper> DirHandle;

inline DirHandle getHandle(const std::string& path)
{
    return {new DirHandleWrapper{path, nullptr},
            [](DirHandleWrapper* handle)
            {
                if (handle->handle != nullptr && handle->handle != INVALID_HANDLE_VALUE)
                {
                    FindClose(handle->handle);
                }
                delete handle;
            }};
}

inline std::string getNextItem(DirHandle handle)
{
    WIN32_FIND_DATAA data;
    while (true)
    {
        if (handle->handle == nullptr)
        {
            handle->handle = FindFirstFileA((handle->path + "\\*").c_str(), &data);
            if (handle->handle == INVALID_HANDLE_VALUE)
            {
                std::cout << "dupa " << GetLastError() << '\n';
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
}

#endif // DIRCRAWL_WINAPI_H
