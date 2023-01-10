#include "EventLoop.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include "Logger.h"
#include <errno.h>
#include "Poller.h"
#include "Channel.h"
//防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

//创建wakeupfd， 用来notify唤醒subReactor处理醒来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p in thread %d \n", this, threadId_);
    }
    else{
        t_loopInThisThread = this;
    }
    //设置wakeupfd的事件类型及之后发生事件的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    
    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);//往activeChannels_中添加监听到的channel
        for(auto channel : activeChannels_)
        {
            //poller监听哪些channel发生事件，上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //wakeup subloop后，执行下面的方法
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}

//退出事件循环 1.loop在自己的线程中调用quit 2.在其他线程中调用quit
void EventLoop::quit()
{
    quit_ = true;

    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    //唤醒相应的需要执行cb的loop的线程了
    //callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop有新的回调，将loop唤醒
    if(!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}
//向wakeupfd写一个数据,wakeupChannel发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if( n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }

}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const auto &functor : functors)
    {
        functor();//执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if( n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8 \n", static_cast<int>(n));
    }
}