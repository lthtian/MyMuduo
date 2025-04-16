#pragma once

#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Accept.h"
#include "InetAddress.h"
#include "UnCopyable.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include "Logger.h"

#include <atomic>
#include <unordered_map>

class TcpServer : UnCopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option // 表明对端口是否可重用
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string name, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback &cb) { _threadInitCallback = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { _connectionCallback = cb; }
    void setMessageCallback(const MessageCallback &cb) { _messageCallback = cb; }
    void setWriteCommpleteCallback(const WriteCompleteCallback &cb) { _writeCompleteCallback = cb; }

    void setThreadNum(int numThreads) { _threadPool->setThreadNum(numThreads); }

    // 开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *_mainloop;
    const std::string _ipPort;
    const std::string _name;

    std::unique_ptr<Acceptor> _acceptor;
    std::shared_ptr<EventLoopThreadPool> _threadPool; // one loop per thread

    ConnectionCallback _connectionCallback;
    MessageCallback _messageCallback;
    WriteCompleteCallback _writeCompleteCallback;

    ThreadInitCallback _threadInitCallback;

    std::atomic_int _started;

    int _nextConnId;
    ConnectionMap _connections;
};