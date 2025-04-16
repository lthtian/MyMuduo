#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

// 线程局部全局变量指针
// 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThread = nullptr;

// 定义默认IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd, 用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int efd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (efd < 0)
        LOG_FATAL("eventfd error: %d \n", errno);
    return efd;
}

// 一个线程启用一个EventLoop, 一个EventLoop在创立之初确立一个该线程该loop专属的_weakfd
EventLoop::EventLoop()
    : _looping(false), _quit(false), _callingPendingFunctors(false), _threadId(CurrentThread::tid()), _poller(Poller::newDefaultPoller(this)), _wakeupFd(createEventfd()), _wakeupChannel(new Channel(this, _wakeupFd))
{
    if (t_loopInThread)
        LOG_FATAL("Another EventLoop %p exists int this thread %d \n", t_loopInThread, _threadId);
    else
        t_loopInThread = this;

    // 设置wakeupfd的读事件回调
    _wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 使当前loop监听_wakeupfd的EPOLLIN读事件
    _wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
    _wakeupChannel->disableAll();
    _wakeupChannel->remove();
    ::close(_wakeupFd);
    t_loopInThread = nullptr;
}

// 触发这个事件只是为了触发离开循环后的回调
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(_wakeupFd, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_FATAL("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

void EventLoop::loop()
{
    _looping = true;
    _quit = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!_quit)
    {
        _activeChannels.clear();
        _pollReturnTime = _poller->poll(kPollTimeMs, &_activeChannels);
        // 处理自己本身监听的事件
        for (Channel *channel : _activeChannels)
        {
            // 通知每个channel处理对应的事件
            channel->handleEvent(_pollReturnTime);
        }
        // 处理mainLoop/其他subLoop发配给自己的任务(注册新channel, 修改channel)
        // 执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping \n", this);
    _looping = false;
}

void EventLoop::quit()
{
    _quit = true;
    // 要判断当前工作线程是不是IO线程, 如果不是, 则唤醒主线程
    // 由于_quit线程是共享资源的, 在工作线程修改的_quit会在IO线程产生效果, 从而真正在主线程quit
    if (!isInLoopThread())
        wakeup();
}

// Channel -> EventLoop -> Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    _poller->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    _poller->removeChannel(channel);
}
bool EventLoop::hansChannel(Channel *channel)
{
    return _poller->hasChannel(channel);
}

// 在当前的loop执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
        cb();
    else
        queueInLoop(cb);
}

// 把cb放入队列中, 唤醒loop所在的线程, 执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _pendingFunctors.emplace_back(cb);
    }

    // 唤醒相应loop
    // 不在对应线程 | 在对应线程但是正在执行回调(执行完会回到阻塞, 可用wakeup触发)
    if (!isInLoopThread() || _callingPendingFunctors)
        wakeup();
}

// 唤醒loop所在的线程 向wakeupfd写一个数据
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(_wakeupFd, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    _callingPendingFunctors = true;

    {
        std::unique_lock<std::mutex> lock(_mutex);
        functors.swap(_pendingFunctors);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    _callingPendingFunctors = false;
}