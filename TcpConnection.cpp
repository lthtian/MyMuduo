#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <memory>
#include <unistd.h>

// 为什么用static可防止名字冲突
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
        LOG_FATAL("%s:%s:%d mainLoop is null!", __FILE__, __FUNCTION__, __LINE__);
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string name, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : _loop(CheckLoopNotNull(loop)), _name(name), _state(kConnecting), _reading(true), _socket(new Socket(sockfd)), _channel(new Channel(loop, sockfd)), _localAddr(localAddr), _peerAddr(peerAddr), _highWaterMark(64 * 1024 * 1024) // 64MB
{
    // 把回调设置进channel, TcpConnection有一套自己的回调函数
    _channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    _channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    _channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    _channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", _name.c_str(), sockfd);
    _socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("Tcpconnection::dtor[%s] at fd=%d state=%d \n", _name.c_str(), _channel->fd(), (int)_state);
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = _inputBuffer.readFd(_channel->fd(), &savedErrno);
    if (n > 0)
    {
        // 有可读事件发生, 调用用户传入的回调操作
        _messageCallback(shared_from_this(), &_inputBuffer, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (_channel->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = _outputBuffer.writeFd(_channel->fd(), &savedErrno);
        if (n > 0)
        {
            _outputBuffer.retrieve(n);
            if (_outputBuffer.readableBytes() == 0) // 已经发送完成
            {
                _channel->disableWriting();
                if (_writeCompleteCallback)
                {
                    _loop->queueInLoop(std::bind(_writeCompleteCallback, shared_from_this()));
                }
                if (_state == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
            LOG_ERROR("TcpConnection::handleWrite");
    }
    else
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", _channel->fd());
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d state=%d \n", _channel->fd(), (int)_state);
    setState(kDisconnected);
    _channel->disableAll();

    // 获取当前的Connention对象
    TcpConnectionPtr connPtr(shared_from_this());
    _connectionCallback(connPtr);
    _closeCallback(connPtr); // 关闭连接, 执行TcpServer::removeConnection
}

void TcpConnection::handleError()
{
    int op;
    socklen_t optlen = sizeof op;
    int err = 0;
    if (::getsockopt(_channel->fd(), SOL_SOCKET, SO_ERROR, &op, &optlen) < 0)
    {
        err = errno;
    }
    else
        err = op;

    LOG_ERROR("TcpConnection::handleErrno name:%s - SO_ERROR:%d \n", _name.c_str(), err);
}

void TcpConnection::connectEstablished() // 连接建立
{
    setState(kConnected);
    // 把自己的指针传给负责的channel, 让其知晓自己的存在状态
    // 防止上层TcpConnection析构后下层channel依旧运行
    // 根本原因是TcpConnection要给到用户手里, 不确定何时析构
    _channel->tie(shared_from_this());
    _channel->enableReading(); // 注册Channel读事件

    // 新连接已经建立, 执行连接建立回调
    _connectionCallback(shared_from_this());
}

void TcpConnection::connectDestoryed() // 连接销毁
{
    if (_state == kConnected)
    {
        setState(kDisconnected);
        _channel->disableAll();
        _connectionCallback(shared_from_this());
    }
    _channel->remove();
}

// 发送数据
void TcpConnection::send(const std::string buf)
{
    if (_state == kConnected)
    {
        if (_loop->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            _loop->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

// 发送数据时, 应用写的快, 而内核发送数据慢, 需要把待发送的数据写入缓冲区并设置水位回调
// 该函数实现对写事件的缓冲
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrite = 0;
    size_t remaining = len;
    bool faultError = false;
    // 之前调用过shutdown
    if (_state == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }
    // _channel第一次开始写数据, 并且缓冲区没有待发送数据
    if (!_channel->isWriting() && _outputBuffer.readableBytes() == 0)
    {
        nwrite = ::write(_channel->fd(), data, len);
        if (nwrite >= 0)
        {
            remaining = len - nwrite;
            // 如果已经直接发送完了, 不需要缓冲, 如果有设置写入完成回调就触发
            if (remaining == 0 && _writeCompleteCallback)
                _loop->queueInLoop(std::bind(_writeCompleteCallback, shared_from_this()));
        }
        else // nwrote < 0
        {
            nwrite = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)
                    faultError = true;
            }
        }
    }
    // 一次write并没有一次发送出去, 需要保存到缓冲区
    // 要给channel设置epollout事件, poller发现tcp发送缓冲区有空间
    // 调用writeCallback -> handleWrite, 把发送缓冲区的数据全部发送完成
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送数据长度
        size_t oldlen = _outputBuffer.readableBytes();
        if (oldlen + remaining >= _highWaterMark && oldlen < _highWaterMark && _highWaterMark)
            _loop->queueInLoop(std::bind(_highWaterMarkCallback, shared_from_this(), oldlen + remaining));
        _outputBuffer.append((char *)data + nwrite, remaining);
        if (!_channel->isWriting())
        {
            _channel->enableWriting(); // 注册channel的写事件, 否则poller不会给channel通知EPOLLOUT,
        }
    }
}

// 关闭Tcp连接
void TcpConnection::shutdown()
{
    if (_state == kConnected)
    {
        setState(kDisconnecting);
        _loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!_channel->isWriting()) // 说明发送缓冲区已经全部发送完成
    {
        _socket->shutdownWrite(); // 关闭写端
        // 之后poller会通知channel触发_closeCallback, 会触发TcpConnection中的handleClose
    }
}