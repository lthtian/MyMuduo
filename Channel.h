#pragma once
#include "UnCopyable.h"
#include "Timestamp.h"
#include <functional>
#include <memory>
class EventLoop;

// 通道, 封装了sockfd和其感兴趣的event, 如EPOLLIN, EPOLLOUT
// 需要和Poller互动, Channel向Poller设置感兴趣的事件, Poller向Channel返回发生的事件

class Channel : UnCopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *Loop, int fd);
    ~Channel();

    // fd得到poller通知以后, 处理事件的函数
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) { _readCallback = std::move(cb); }
    void setWriteCallback(EventCallback cb) { _writeCallback = std::move(cb); }
    void setCloseCallback(EventCallback cb) { _closeCallback = std::move(cb); }
    void setErrorCallback(EventCallback cb) { _errorCallback = std::move(cb); }

    // 防止当channel被手动remove掉后, channel还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const { return _fd; }
    int events() const { return _events; }
    // Channel本身无法取得发生的事件, 是Poller取得发生的事件设置到Channel中的
    void set_revents(int revt) { _revents = revt; }

    // 返回是否关心某种事件或什么都不关心
    bool isNoneEvent() const { return _events == kNoneEvent; }
    bool isWriting() const { return _events & kWriteEvent; }
    bool isReading() const { return _events & kReadEvent; }

    // 设置fd相应的事件状态
    void enableReading() { _events |= kReadEvent, update(); }
    void disableReading() { _events &= ~kReadEvent, update(); }
    void enableWriting() { _events |= kWriteEvent, update(); }
    void disableWriting() { _events &= ~kWriteEvent, update(); }
    void disableAll() { _events = kNoneEvent, update(); }

    int state() { return _state; }
    void set_state(int idx) { _state = idx; }

    // 每个Channel都属于一个EventPool, EventPool可以有多个Channel
    EventLoop *ownerLoop() { return _loop; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // 表示当前fd的状态
    static const int kNoneEvent;  // 没有对任何事件感兴趣
    static const int kReadEvent;  // 对读事件感兴趣
    static const int kWriteEvent; // 对写事件感兴趣

    EventLoop *_loop; // 事件循环
    const int _fd;    // 监听对象
    int _events;      // 注册fd感兴趣的事件
    int _revents;     // fd上发生的事件
    int _state;

    // 防止回调函数在 Channel 所绑定的对象已析构的情况下仍然被调用
    std::weak_ptr<void> _tie;
    bool _tied;

    // 因为channel通道里可以获知fd发生的具体事件, 所以其负责调用具体的事件回调
    ReadEventCallback _readCallback;
    EventCallback _writeCallback;
    EventCallback _closeCallback;
    EventCallback _errorCallback;
};