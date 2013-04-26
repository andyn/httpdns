#ifndef FILE_HH
#define FILE_HH

#include <errno.h>
#include <limits.h>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "filedescriptor.hh"

class File : public FileDescriptor {
public:
    File(std::string const &path, int flags, int mode = 0)
        : FileDescriptor(::open(path.c_str(), flags, mode)) {}

private:
    /// Noncopyable, as per RO3
    File(File const &); // RO3
    /// Nonassignable, as per RO3
    File &operator=(File const &); // RO3

protected:
};

#endif
