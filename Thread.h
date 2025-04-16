#pragma once

#include "UnCopyable.h"
#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <atomic>
#include <string>

// 一个Thread对象记录的就是一个新线程的详细信息

class Thread : UnCopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return _started; }
    pid_t tid() const { return _tid; }
    const std::string &name() { return _name; }

private:
    void setDefaultName();

    bool _started;
    bool _joined;
    std::shared_ptr<std::thread> _thread;
    pid_t _tid;
    ThreadFunc _func;
    std::string _name;
    static std::atomic_int32_t _numCreated; // 记录产生的线程个数
};