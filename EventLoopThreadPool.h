#pragma once
#include "UnCopyable.h"
#include "EventLoopThread.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThreadPool : UnCopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *mainloop, const std::string &name);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { _numThreads = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果工作在多线程中, mainLoop以轮循的方式分配channel给subloop
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();
    bool started() const { return _started; }
    const std::string name() const { return _name; }

private:
    EventLoop *_mainLoop;
    std::string _name;
    bool _started;
    int _numThreads;
    int _next;
    std::vector<std::unique_ptr<EventLoopThread>> _threads;
    std::vector<EventLoop *> _loops;
};