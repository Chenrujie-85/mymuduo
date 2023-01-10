#pragma once

#include "noncopyable.h"
#include <functional>
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include <sys/eventfd.h>
#include <mutex>
#include "CurrentThread.h"
#include <memory>

class Channel;
class Poller;
//包含两个模块，channel 和poller(epoll的抽象)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();
    //开启事件循环
    void loop();
    //退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    //在当前loop中执行
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程执行cb
    void queueInLoop(Functor cb);
    //唤醒loop所在线程
    void wakeup();
    //EventLoop的方法=》poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);
    //判断EventLoop对象是否在自己的线程里
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
private:
    void handleRead();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; //原子操作，通过CAS实现
    std::atomic_bool quit_; //标志退出loop循环
    
    const pid_t threadId_; //纪录当前loop所在线程的id
    Timestamp pollReturnTime_; //poller返回发生事件时的时间
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; //当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒一个subloop
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; //表示当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; //存储loop需要执行的所有回调操作
    std::mutex mutex_;
};