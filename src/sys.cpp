#include "sys.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace binstore
{

namespace internal
{

int SysImpl::open(const char* pathname, int flags) const
{
    return ::open(pathname, flags);
}

int SysImpl::close(int fd) const
{
    return ::close(fd);
}

off_t SysImpl::lseek(int fd, off_t offset, int whence) const
{
    return ::lseek(fd, offset, whence);
}

ssize_t SysImpl::read(int fd, void* buf, size_t count) const
{
    return ::read(fd, buf, count);
}

ssize_t SysImpl::write(int fd, const void* buf, size_t count) const
{
    return ::write(fd, buf, count);
}

int SysImpl::fstat(int fd, struct stat* buf) const
{
    return ::fstat(fd, buf);
}

int SysImpl::stat(const char* pathname, struct stat* buf) const
{
    return ::stat(pathname, buf);
}

SysImpl sys_impl;

} // namespace internal

} // namespace binstore
