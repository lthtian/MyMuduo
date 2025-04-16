#pragma once

#include "UnCopyable.h"
#include "InetAddress.h"

class Socket : UnCopyable
{
public:
    explicit Socket(int sockfd) : _sockfd(sockfd) {}
    ~Socket();

    int fd() const { return _sockfd; }
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

public:
    const int _sockfd;
};