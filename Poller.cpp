#include "Poller.h"

// 判断参数channel是否在当前Poller中
bool Poller::hasChannel(Channel *channel) const
{
    auto it = _channels.find(channel->fd());
    return it != _channels.end() && it->second == channel;
}