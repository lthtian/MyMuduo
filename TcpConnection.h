#pragma once
#include "UnCopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>
#include <string>

class Channel;
class EventLoop;
class Socket;

// TcpServer -> Acceptor -> 新连接 -> accept得到connfd -> 打包回调到TcpConnection
// -> 回调设置给Channel -> Poller -> Channel的回调操作

// 各种回调函数 : 用户 -> TcpServer -> TcpConnection -> Channel

class TcpConnection : UnCopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string name, int sockfd, const InetAddress &LocalAddr, const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return _loop; }
    const std::string &name() const { return _name; }
    const InetAddress &localAddress() const { return _localAddr; }
    const InetAddress &peerAddress() const { return _peerAddr; }

    bool connected() const { return _state == kConnected; }

    void setConnectionCallback(const ConnectionCallback &cb) { _connectionCallback = cb; }
    void setMessageCallback(const MessageCallback &cb) { _messageCallback = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { _writeCompleteCallback = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { _highWaterMarkCallback = cb; }
    void setCloseCallback(const CloseCallback &cb) { _closeCallback = cb; }

    void connectEstablished(); // 连接建立
    void connectDestoryed();   // 连接销毁

    // 发送数据
    void send(std::string buf);
    // 关闭Tcp连接
    void shutdown();

private:
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *message, size_t len);
    void shutdownInLoop();

    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    void setState(StateE state) { _state = state; }

    EventLoop *_loop; // 这里只会是subLoop
    const std::string _name;
    std::atomic_int _state;
    bool _reading;
    // 管理的_socket / _channel
    std::unique_ptr<Socket> _socket;
    std::unique_ptr<Channel> _channel;

    const InetAddress _localAddr; // 主机addr
    const InetAddress _peerAddr;  // 对端客户addr
    // 回调
    ConnectionCallback _connectionCallback;
    MessageCallback _messageCallback;
    WriteCompleteCallback _writeCompleteCallback;
    HighWaterMarkCallback _highWaterMarkCallback;
    CloseCallback _closeCallback;
    size_t _highWaterMark;
    // 缓冲区
    Buffer _inputBuffer;
    Buffer _outputBuffer;
};