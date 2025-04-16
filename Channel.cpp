#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *Loop, int fd)
    : _loop(Loop), _fd(fd), _events(0), _revents(0), _state(-1), _tied(false)
{
}

Channel::~Channel()
{
}

// 一个TcpConnection新连接创建的时候, 接受TcpConnection传来的一个指针
// 用这个指针可以随时查看对应TcpConnection对象是否还在
void Channel::tie(const std::shared_ptr<void> &obj)
{
    _tie = obj;
    _tied = true;
}

void Channel::update()
{
    // 通过channel所属的EventLoop, 调用poller对应的方法, 注册fd的事件
    _loop->updateChannel(this);
}

// 在channel所属的EventLoop中删除该channel
void Channel::remove()
{
    _loop->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (_tied)
    {
        std::shared_ptr<void> guard = _tie.lock();
        if (guard)
            handleEventWithGuard(receiveTime);
    }
    else
        handleEventWithGuard(receiveTime);
}

// 根据当前设置的事件执行相应的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channle handleEvent revents: %d\n", _revents);
    if ((_revents & EPOLLHUP) && !(_revents & EPOLLIN))
    {
        // 回调不为空则执行回调
        if (_closeCallback)
            _closeCallback();
    }

    if (_revents & EPOLLERR)
    {
        if (_errorCallback)
            _errorCallback();
    }

    if (_revents & EPOLLOUT)
    {
        if (_writeCallback)
            _writeCallback();
    }

    if (_revents & (EPOLLIN | EPOLLPRI))
    {
        if (_readCallback)
            _readCallback(receiveTime);
    }
}
