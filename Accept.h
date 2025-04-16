#pragma once

#include "UnCopyable.h"
#include "Channel.h"
#include "Socket.h"
#include "InetAddress.h"

#include <functional>

class EventLoop;

class Acceptor : UnCopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) { _newConnectionCallback = move(cb); }

    bool listenning() const { return _listenning; }
    void listen();

private:
    void handleRead();

    EventLoop *_loop;
    Socket _acceptSocket;
    Channel _acceptChannel;
    NewConnectionCallback _newConnectionCallback;
    bool _listenning;
};