#pragma once

#include "Channel.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

// muduo库中多路事件分发器中的核心, 用于触发IO复用
// 此层为抽象基类, 用于作为Epoll和Poll的基类

class Poller : UnCopyable
{
public:
    using ChannelList = std::vector<Channel *>;
    Poller(EventLoop *loop) : _ownerLoop(loop) {}
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel是否在当前Poller中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认Poller
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    using ChannelMap = std::unordered_map<int, Channel *>;
    // 每个loop真正是在这里维护监听的channel
    ChannelMap _channels;

public:
    EventLoop *_ownerLoop;
};