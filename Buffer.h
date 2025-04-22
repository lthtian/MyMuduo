#pragma once

#include <vector>
#include <string>
#include <sys/uio.h>

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : _buffer(kCheapPrepend + initialSize), _readerIndex(kCheapPrepend), _writerIndex(kCheapPrepend)
    {
    }

    size_t readableBytes() const { return _writerIndex - _readerIndex; }
    size_t writableBytes() const { return _buffer.size() - _writerIndex; }
    size_t prependableBytes() const { return _readerIndex; }
    // 返回缓冲区中可读区域的起始地址
    const char *peek() const { return begin() + _readerIndex; }

    void retrieve(size_t len)
    {
        if (len < readableBytes())
            _readerIndex += len; // 只读取了可读缓冲区的一部分
        else
            retrieveAll();
    }

    void retrieveAll()
    {
        _readerIndex = _writerIndex = kCheapPrepend; // 全读了, 复位
    }

    // Buffer -> string
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }

    std::string retrieveAsString(size_t len)
    {
        std::string res(peek(), len);
        retrieve(len);
        return res;
    }

    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
            makeSpace(len);
    }

    // 向缓冲区中追加数据[data, data + len]
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        _writerIndex += len;
    }

    char *beginWrite() { return begin() + _writerIndex; }
    const char *beginWrite() const { return begin() + _writerIndex; }

    // 从fd上读取数据, Poller工作在LT模式
    // Buffer缓冲区是有大小的, 但从fd上读数据时却不知道tcp数据最终的大小
    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin() { return &*_buffer.begin(); }
    const char *begin() const { return &*_buffer.begin(); }

    void makeSpace(size_t len)
    {
        // 后面真正可写的 + 前面读出来空余的 < 要求的大小
        if (writableBytes() + _readerIndex < len + kCheapPrepend)
            _buffer.resize(_writerIndex + len);
        else // 如果够, 把后面未读的移到前面, 给后面空出来
        {
            size_t readable = readableBytes();
            std::copy(begin() + _readerIndex, begin() + _writerIndex, begin() + kCheapPrepend);
            _readerIndex = kCheapPrepend;
            _writerIndex = kCheapPrepend + readable;
        }
    }

    std::vector<char> _buffer;
    size_t _readerIndex;
    size_t _writerIndex;
};