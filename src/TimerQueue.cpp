#include "TimerQueue.h"
#include "Logger.h"
#include "EventLoop.h"
#include "Timer.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

static int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return timerfd;
}

//将 TimeStamp 类型转化为 timespec 类型
struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                            - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(
        microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(
        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
    return ts;
}

void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    //调用 read 清除缓冲区的内容
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_INFO("TimerQueue::handleRead() %d at %s \n", static_cast<int>(howmany), now.toString().c_str());
    if (n != sizeof howmany)
    {
        LOG_ERROR("TimerQueue::handleRead() reads %d bytes instead of 8\n", static_cast<int>(n));
    }
}

//设置新的到期时间，时间到了则触发时间，当1.有新的事件插入时addTimer()，2.过期事件被删除时reset()
void resetTimerfd(int timerfd, Timestamp expiration)
{
  // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    //设置定时器的到时时间，最早的时间戳
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret)
    {
        LOG_INFO("timerfd_settime, timerfd = %d \n", timerfd);
    }
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop)
    , timerfd_(createTimerfd())
    , timerfdChannel_(loop, timerfd_)
    , timers_()
    , callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));//设置触发后的回调函数
    timerfdChannel_.enableReading();//该函数将timerfd绑定给loop的fd
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    for(auto iter = timers_.begin(); iter != timers_.end(); ++iter)
    {
        delete iter->second;
    }
}

//对于第一次或者列表为空时添加 timer 对象，就是将 timer 加入到列表中，然后设置超时时间；当超时时间到来时触发可读事件；
TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

//当添加时间时由addTimer调用
void TimerQueue::addTimerInLoop(Timer* timer)
{
    bool earliestChanged = insert(timer);
    //插入一个定时器，插入的定时器的超时时间可能会比当前定时器列表的最早超时时间还要早
    //如果是这样的话，就需要进行一个调整
    if(earliestChanged)
    {
        //重置定时器的超时时刻
        resetTimerfd(timerfd_, timer->expiration());
    }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    auto iter = activeTimers_.find(timer);
    if(iter != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(iter->first->expiration(), iter->first));
        if(n != 1)
        {
            LOG_ERROR("erase timer error, timerseq = %d \n", static_cast<int>(timerId.sequence_));
        }
        delete iter->first;
        activeTimers_.erase(iter);
    }
    //如果不在定时器当中，有两种情况，一是根本没有，二是已经到期了
    //加入到 cancelingTimers_ 中，如果是周期性的也会被删除（reset）
    else if(callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
}

//当有事件到期则调用该函数
void TimerQueue::handleRead()
{
    Timestamp now(Timestamp::now());
    //可读事件到来时，一定要将内核输入缓冲区的内容读取
    //否则如果是 PollPoller 则可读事件一直处于高电平，会一直触发可读事件
    //如果是 EPollPoller 的 LT 和上面情况一样
    //如果是 ET，则下一次事件到来时还是高电平，可读事件不会被触发
    readTimerfd(timerfd_, now);
    //定时器在同一时间戳下可能存在多个 Timer 对象，所以要将所有超时 Timer 对象取出
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for (auto iter = expired.begin(); iter != expired.end(); ++iter)
    {
        //运行取消定时器的回调函数
        iter->second->run();
    }
    callingExpiredTimers_ = false;
    //如果不是一次性定时器，重置
    reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    //这里设置 sentry 的地址为最大值【全 1】
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    auto end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    for (auto iter = expired.begin(); iter != expired.end(); ++iter)
    {
        ActiveTimer timer(iter->second, iter->second->sequence());
        size_t n = activeTimers_.erase(timer);
        if(n != 1)
        {
            LOG_ERROR("erase timer error, timerseq = %d \n", static_cast<int>(timer.first->sequence()));
        }
    }
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;

    for(auto iter = expired.begin(); iter != expired.end(); ++iter)
    {
        ActiveTimer timer(iter->second, iter->second->sequence());
        if(iter->second->repeat())
        {
            iter->second->restart(now);
            insert(iter->second);
        }
        else
        {
            delete iter->second;
        }
    }
    if(!timers_.empty())
    {
        nextExpire = timers_.begin()->second->expiration();
    }

    if(nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}

bool TimerQueue::insert(Timer* timer)
{
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    //最早到期时间是否改变，一开始没有改变
    auto iter = timers_.begin();
    if(iter == timers_.end() || when < iter->first)
    {
        earliestChanged = true;
    }

    timers_.insert(Entry(when, timer));

    activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    return earliestChanged;
}