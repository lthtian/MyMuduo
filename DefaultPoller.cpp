#include "Poller.h"
#include "EPollPoller.h"
#include <stdlib.h>

// EventLoop可以通过该接口获取默认Poller
Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
        return nullptr;
    else
        return new EPollPoller(loop);
}