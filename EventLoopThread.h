#pragma once

#include "UnCopyable.h"
#include "EventLoop.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>

class EventLoopThread : UnCopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *_loop;
    bool _exiting;
    Thread _thread;
    std::mutex _mutex;
    std::condition_variable _cond;
    ThreadInitCallback _cb; // ?
};