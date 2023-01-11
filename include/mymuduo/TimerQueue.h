#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"
#include <mutex>
#include <set>
#include "TimerId.h"
#include <vector>
#include <atomic>


class EventLoop;
class Timer;

class TimerQueue : noncopyable
{
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);

    void cancel(TimerId timerId);

private:
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    //当发生超时事件时的回调函数
    void handleRead();
    //移除所有的超时事件
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    EventLoop* loop_;//定时器所属的Loop循环
    const int timerfd_;//定时器的文件描述符
    Channel timerfdChannel_;//定时器绑定的channel
    TimerList timers_;

    ActiveTimerSet activeTimers_;
    std::atomic_bool callingExpiredTimers_;//是否在处理超时的定时器
    ActiveTimerSet cancelingTimers_;//保存被取消的定时器
};