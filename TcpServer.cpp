#include "TcpServer.h"
#include "Logger.h"

#include <strings.h>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
        LOG_FATAL("%s:%s:%d mainLoop is null!", __FILE__, __FUNCTION__, __LINE__);
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string name, Option option)
    : _mainloop(CheckLoopNotNull(loop)), _ipPort(listenAddr.toIpPort()), _name(name), _acceptor(new Acceptor(loop, listenAddr, option == kReusePort)), _threadPool(new EventLoopThreadPool(loop, name)), _connectionCallback(), _messageCallback(), _nextConnId(1), _started(0)
{
    // 当有新用户连接时, 会执行newConnection
    _acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                  std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : _connections)
    {
        TcpConnectionPtr conn(item.second); // 局部强智能指针对象会自动释放
        item.second.reset();                // 原map中放弃对强指针的使用

        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
    }
}

void TcpServer::start()
{
    if (_started++ == 0) // 防止被重复启动多次
    {
        _threadPool->start(_threadInitCallback);
        _mainloop->runInLoop(std::bind(&Acceptor::listen, _acceptor.get()));
    }
}

// 在TcpServer中具体是轮询找到subLoop, 唤醒该loop
// 把sockfd封装成channel加入到subLoop中
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *subLoop = _threadPool->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", _ipPort.c_str(), _nextConnId);
    ++_nextConnId;
    std::string connName = _name + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
             _name.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    // 通过sockfd直接获取绑定的ip+port
    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
        LOG_ERROR("sockets::getLocalAddr");

    InetAddress localAddr(local);

    // 根据连接成功的sockfd, 创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(subLoop, connName, sockfd, localAddr, peerAddr));
    // 存储连接名字与对应连接的映射
    _connections[connName] = conn;
    // 下面的回调都是用户设置给TcpServer的
    conn->setConnectionCallback(_connectionCallback);
    conn->setMessageCallback(_messageCallback);
    conn->setWriteCompleteCallback(_writeCompleteCallback);
    // 设置了如何关闭连接的回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    // 直接调用TcpConnection::connectEstablished
    // 代表连接建立成功
    subLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    _mainloop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
             _name.c_str(), conn->name().c_str());

    _connections.erase(conn->name());
    EventLoop *subloop = conn->getLoop();
    subloop->queueInLoop(std::bind(&TcpConnection::connectDestoryed, conn));
}