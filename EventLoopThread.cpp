#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : _loop(nullptr), _exiting(false), _thread(std::bind(&EventLoopThread::threadFunc, this), name), _mutex(), _cond(), _cb(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    _exiting = true;
    if (_loop != nullptr)
    {
        _loop->quit();
        _thread.join();
    }
}

// 获取一个新线程中单独运行的EventLoop对象指针
// 利用条件变量 + 共享内存实现了线程间通信
EventLoop *EventLoopThread::startLoop()
{
    _thread.start();

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_loop == nullptr)
        {
            _cond.wait(lock);
        }
        loop = _loop;
    }
    return loop;
}

// 在单独的新线程里面运行
void EventLoopThread::threadFunc()
{
    // one loop per thread
    EventLoop loop;
    if (_cb)
    {
        _cb(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(_mutex);
        _loop = &loop;
        _cond.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(_mutex);
    _loop = nullptr;
}