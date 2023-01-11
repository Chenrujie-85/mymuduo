#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include <atomic>

class Timer : noncopyable
{
public:
    Timer(const TimerCallback& cb, Timestamp when, double interval)
        : timerCallback_(cb)
        , expiration_(when)
        , interval_(interval)
        , repeat_(interval > 0.0)
        , sequence_(++s_numCreated_)
    {}

    void run() const
    {
        timerCallback_();
    }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);

    static int64_t numCreated() { return s_numCreated_; }

private:
    TimerCallback timerCallback_;//定时回调函数
    Timestamp expiration_;//下一次的超时时刻
    const double interval_;//超时时间间隔，如果是一次性定时器则设置为0
    const bool repeat_;//该定时器是否可以重复
    const int64_t sequence_;//定时期的序号

    static std::atomic_int s_numCreated_;//已经创建的定时器的个数
};
