#include <mymuduo/TcpServer.h>
// #include "../Logger.h"

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : _server(loop, addr, name), _loop(loop)
    {
        // 注册回调函数
        _server.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        _server.setMessageCallback(
            std::bind(&EchoServer::onMessage, this,
                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // 设置合适的loop线程数量
        _server.setThreadNum(3);
    }

    void start()
    {
        _server.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();
    }

    EventLoop *_loop;
    TcpServer _server;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8080);
    EchoServer server(&loop, addr, "hello");
    server.start(); // listen,
    loop.loop();    // 启动mainLoop中底层的Poller

    return 0;
}