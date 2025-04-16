#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <cstring>

// 有一个channel还没有添加到poller里, 与channel的成员_index初始值相同
const int kNew = -1;
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), _epollfd(::epoll_create1(EPOLL_CLOEXEC)) // 子进程继承的epid会在调用exec后关闭
      ,
      _events(kInitEventListSize) // vector初始长度设置为16
{
    if (_epollfd < 0)
        LOG_FATAL("epoll_create error: %d\n", errno);
}

EPollPoller::~EPollPoller()
{
    ::close(_epollfd);
}

// virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

// 对应epoll_ctl
void EPollPoller::updateChannel(Channel *channel)
{
    const int state = channel->state();
    LOG_INFO("func=%s fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), state);

    if (state == kNew || state == kDeleted)
    {
        if (state == kNew)
        {
            int fd = channel->fd();
            _channels[fd] = channel;
        }
        channel->set_state(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        // 不是新增, 如果发现fd已经没有关心的事件, 就直接取消对fd的监视
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_state(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::update(int operation, Channel *channel)
{
    // 这里通过ctl将event存入内核中, 之后会通过wait把data原封不动地返回回来
    epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();

    if (::epoll_ctl(_epollfd, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        else
            LOG_FATAL("epoll_ctl add/mod: %d\n", errno);
    }
}

void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    _channels.erase(fd);

    LOG_INFO("func=%s fd=%d\n", __FUNCTION__, fd);

    int state = channel->state();
    if (state == kAdded)
        update(EPOLL_CTL_DEL, channel);
    channel->set_state(kNew);
}

// 通过epoll_wait监听到哪些事件发生, 并把发生的事件填入EventLoop提供的ChannelList中
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    LOG_INFO("func=%s => fd total count: %lu \n", __FUNCTION__, _channels.size());
    // epoll_wait第二个参数要求原生数组, 但是用下面的方式可以改为使用vector, 便于扩容
    int numEvents = ::epoll_wait(_epollfd, &*_events.begin(), static_cast<int>(_events.size()), timeoutMs);
    int saveErron = errno; // errno是全局变量, 可以先存起来防止线程问题
    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        // 如果监听到发生的事件数量已经等于数组大小
        // 说明有可能更多, 需要扩容
        if (numEvents == _events.size())
        {
            _events.resize(_events.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if (saveErron != EINTR)
        {
            errno = saveErron;
            LOG_ERROR("poll() error!");
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    // 遍历返回的活跃事件, 将每个事件存入EventLoop的活跃数组
    for (size_t i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel *>(_events[i].data.ptr);
        channel->set_revents(_events[i].events);
        activeChannels->push_back(channel);
    }
}