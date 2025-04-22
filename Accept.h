// Acceptor.h
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

    EventLoop *_loop;                             // 用来标注这个Acceptor属于的mainLoop
    Socket _acceptSocket;                         // 存储listensocketfd
    Channel _acceptChannel;                       // 对listensocketfd进行封装的Channel, 便于注册读事件
    NewConnectionCallback _newConnectionCallback; // TcpServer给其设置的新连接回调函数
    bool _listenning;                             // 是否正在监听
};