#pragma once
#include "Poller.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    virtual void updateChannel(Channel *channel) override;
    virtual void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    // epoll_wait得到活跃的事件进行填入
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新epoll的内核事件表, 就是使用epoll_ctl
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;
    int _epollfd;
    EventList _events; // 存储发生的事件
};