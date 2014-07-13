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

* `listFiles(const std::string& path)` - returns an iterable object listing files in given path (absolute or relative to working directory)
* `listDirectories(const std::string& path)` - returns an iterable object listing subdirectories in given path (absolute or relative to working directory)
