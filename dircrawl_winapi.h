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
#include <cwchar>
#include <windows.h>
#include <utf8.h>

#include "dircrawl_types.h"

namespace dirc {
namespace platform {

char separator = '\\';

struct DirHandleWrapper
{
    std::wstring path;
    HANDLE handle;
};
typedef std::shared_ptr<DirHandleWrapper> DirHandle;

inline std::wstring preparePath(const std::string& path)
{
    std::wstring path_utf16;
    try
    {
        utf8::utf8to16(path.begin(), path.end(), std::back_inserter(path_utf16));
    }
    catch (utf8::exception)
    {
        return {};
    }
    for (wchar_t& c: path_utf16)
    {
        if (c == L'/')
        {
            c = L'\\';
        }
    }
    return path_utf16;
}

inline DirHandle getHandle(const std::string& path)
{
    std::wstring path_utf16 = preparePath(path);
    if (path_utf16.empty())
    {
        return nullptr;
    }
    else
    {
        return {new DirHandleWrapper{path_utf16, nullptr},
                [](DirHandleWrapper* handle)
                {
                    if (handle->handle != nullptr && handle->handle != INVALID_HANDLE_VALUE)
                    {
                        FindClose(handle->handle);
                    }
                    delete handle;
                }};
    }
}

inline std::string getNextItem(DirHandle handle)
{
    WIN32_FIND_DATAW data;
    while (true)
    {
        if (handle->handle == nullptr)
        {
            handle->handle = FindFirstFileW((handle->path + L"\\*").c_str(), &data);
            if (handle->handle == INVALID_HANDLE_VALUE)
            {
                return {};
            }
        }
        else
        {
            if (!FindNextFileW(handle->handle, &data))
            {
                return {};
            }
        }
        std::string name;
        utf8::unchecked::utf16to8(data.cFileName, data.cFileName + wcslen(data.cFileName),
                std::back_inserter(name));
        if (name != "." && name != "..")
        {
            return name;
        }
    }
}

inline EntryType getType(const std::string& path)
{
    std::wstring path_utf16 = preparePath(path);
    if (path_utf16.empty())
    {
        return EntryType::unknown;
    }

    auto type = GetFileAttributesW(path_utf16.c_str());
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
