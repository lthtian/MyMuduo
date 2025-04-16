#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"

#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *mainloop, const std::string &name)
    : _mainLoop(mainloop), _name(name), _started(false), _numThreads(0), _next(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // 线程上绑定的对象都是栈上的的对象, 无需关心析构
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)
{
    _started = true;

    for (int i = 0; i < _numThreads; i++)
    {
        char buf[_name.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", _name.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        _threads.push_back(std::unique_ptr<EventLoopThread>(t));
        _loops.push_back(t->startLoop());
    }

    // 整个服务端只有一个线程, 让mainloop来运行
    if (_numThreads == 0 && cb)
    {
        cb(_mainLoop);
    }
}

// 如果工作在多线程中, mainLoop以轮询的方式分配channel给subloop
EventLoop *EventLoopThreadPool::EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = _mainLoop;

    // 轮询
    if (!_loops.empty())
    {
        loop = _loops[_next];
        ++_next;
        if (_next >= _loops.size())
            _next = 0;
    }

    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::EventLoopThreadPool::getAllLoops()
{
    if (_loops.empty())
        return std::vector<EventLoop *>(1, _mainLoop);
    else
        return _loops;
}