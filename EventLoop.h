#pragma once

#include "UnCopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

class EventLoop : UnCopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    Timestamp pollReturnTime() const { return _pollReturnTime; }

    // 1
    void loop(); // 开启事件循环
    void quit(); // 退出事件循环

    // 2
    // Channel -> EventLoop -> Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hansChannel(Channel *channel);

    // 3
    // 判断对象是否在自己的线程里
    bool isInLoopThread() const { return _threadId == CurrentThread::tid(); }

    void runInLoop(Functor cb);   // 先判断是否是在自己的线程中, 是就使用回调, 不是就放入队列
    void queueInLoop(Functor cb); // 把cb放入队列中, 唤醒loop所在的线程, 执行cb

    void wakeup();

private:
    void handleRead();        // weak up
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel *>;
    std::atomic_bool _looping;
    std::atomic_bool _quit;                   // 标识退出loop循环
    std::atomic_bool _callingPendingFunctors; // 标识当前loop是否有需要执行的回调操作
    const pid_t _threadId;                    // 记录创建该loop所在的线程id
    Timestamp _pollReturnTime;                // poller返回发生事件的channels的时间点

    std::unique_ptr<Poller> _poller;
    // 由eventfd()创建, 当mainLoop获取一个新用户的channel, 通过轮循算法选择一个subLoop, 唤醒该成员
    int _wakeupFd;
    std::unique_ptr<Channel> _wakeupChannel;

    ChannelList _activeChannels;

    // 这个资源有可能被其他线程访问, 需要上锁
    std::vector<Functor> _pendingFunctors; // 存储loop需要执行的所有回调操作d
    std::mutex _mutex;
};