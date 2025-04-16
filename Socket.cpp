#include "Socket.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>

Socket::~Socket()
{
    ::close(_sockfd);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0 != ::bind(_sockfd, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail \n", _sockfd);
    }
}

void Socket::listen()
{
    if (0 != ::listen(_sockfd, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail \n", _sockfd);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    socklen_t len(sizeof addr);
    // accept4可以直接给接受到的sockfd设置选项(非阻塞)
    int connfd = ::accept4(_sockfd, (sockaddr *)&addr, &len, SOCK_CLOEXEC | SOCK_NONBLOCK);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    ::shutdown(_sockfd, SHUT_WR);
}

void Socket::setTcpNoDelay(bool on)
{
    int op = on ? 1 : 0;
    ::setsockopt(_sockfd, IPPROTO_TCP, TCP_NODELAY, &op, sizeof op);
}

void Socket::setReuseAddr(bool on)
{
    int op = on ? 1 : 0;
    ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof op);
}

void Socket::setReusePort(bool on)
{
    int op = on ? 1 : 0;
    ::setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &op, sizeof op);
}

void Socket::setKeepAlive(bool on)
{
    int op = on ? 1 : 0;
    ::setsockopt(_sockfd, SOL_SOCKET, SO_KEEPALIVE, &op, sizeof op);
}