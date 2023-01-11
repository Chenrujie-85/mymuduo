#include "Timer.h"

class Timestamp;

std::atomic_int Timer::s_numCreated_(0);

//重启计时器
void Timer::restart(Timestamp now)
{
    if(repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}