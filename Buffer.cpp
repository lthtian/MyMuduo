#include "Buffer.h"
#include <unistd.h>

// 从fd上读取数据, Poller工作在LT模式
// Buffer缓冲区是有大小的, 但从fd上读数据时却不知道tcp数据最终的大小
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间 64k
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + _writerIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    // 在非连续的区块中依次写入同一个fd传入的信息
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
        *saveErrno = errno;
    else if (n <= writable) // 已经够了不需要扩容
    {
        _writerIndex += n;
    }
    else // extrabuf中也写入了数据, 需要扩容加进去
    {
        _writerIndex = _buffer.size();
        append(extrabuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    int n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}