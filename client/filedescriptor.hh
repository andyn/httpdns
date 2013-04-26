#ifndef FILEDESCRIPTOR_HH
#define FILEDESCRIPTOR_HH

#include <errno.h>
#include <limits.h>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <vector>

using std::string;
using std::vector;

typedef unsigned char byte;

class FileDescriptor {
public:
    /// @Short construct a file descriptor by wrapping an existing fd
    /// @param fd The file descriptor to use. Must be zero or positive,
    /// else an exception is thrown.
    FileDescriptor(int fd);
    
    /// Virtual destructor, as this class is expected to be extended.
    virtual ~FileDescriptor();
    
    /// @short Return the file descriptor in integer format
    /// A FileDescriptor object may be passed directly to functions that
    /// expect an integer file descriptor, such as fcntl.
    operator int() {
        return m_fd;
    }
    
    /// Read data from the file descriptor into a vector
    /// The vector's current size will be used to define how
    /// much data is to be read
    /// @return how many bytes were read
    ssize_t read(vector<char> &buffer);
    
    /// Write data into the file descriptor from a vector (or from a string)
    /// @return how many bytes were written
    ssize_t write(vector<char> const &buffer);
    ssize_t write(string const &buffer);

    /// Do fcntl operations on the file descriptor
    /// See the man page for fcntl(2)
    int fcntl(int cmd, ...);

private:
    /// Noncopyable, as per RO3
    FileDescriptor(FileDescriptor const &); // RO3
    /// Nonassignable, as per RO3
    FileDescriptor &operator=(FileDescriptor const &); // RO3
    
    /// Write a char buffer to fd
    ssize_t writeCharBuffer(char const *buffer, size_t count);

protected:
    int m_fd;
};

#endif
