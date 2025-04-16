#include "Accept.h"
#include "Logger.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create error:%d", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

// 创建socket, 封装进Channel, 往当前loop的poller中添加
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : _loop(loop), _acceptSocket(createNonblocking()), _acceptChannel(loop, _acceptSocket.fd()), _listenning(false)
{
    _acceptSocket.setReuseAddr(true);
    if (reuseport)
        _acceptSocket.setReusePort(true);
    _acceptSocket.bindAddress(listenAddr);

    // 一旦有一个新用户的连接, 就需要执行回调, connfd -> channel -> subloop
    // 具体回调由TcpServer给出
    _acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor()
{
    _acceptChannel.disableAll();
    _acceptChannel.remove();
}

void Acceptor::listen()
{
    _listenning = true;
    _acceptSocket.listen();
    _acceptChannel.enableReading(); // 把acceptChannel注册到poller中监听读事件
}

// 在listenfd读事件触发时调用
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = _acceptSocket.accept(&peerAddr);
    if (connfd >= 0)
    {
        // 在TcpServer中具体是轮询找到subLoop, 唤醒该loop, 将该sockfd分发到其中
        if (_newConnectionCallback)
            _newConnectionCallback(connfd, peerAddr);
        else
            ::close(connfd);
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept error:%d", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit!", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}