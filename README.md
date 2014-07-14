dircrawl
========

C++11 cross-platform header-only library for searching files in directory.

###Usage

Basic usage:

    for(std::string s: dirc::listFiles("some/folder"))
    {
        // s contains filename
    }

###Functions

* `listFiles(const std::string& path)` - returns an iterable object listing files in given path
* `listDirectories(const std::string& path)` - returns an iterable object listing subdirectories in given path
* `listFilesRecursive(const std::string& path)` - returns an iterable object listing files in given path and all its subdirectories

Note: the path must be absolute or relative to current directory
