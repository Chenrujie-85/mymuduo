#pragma once

#include "noncopyable.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Timestamp.h"
#include <unordered_map>
#include <vector>

//muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller(){};

    //给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeout, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *Channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    //判断参数channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const;

    //EventLoop可以根据该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    //map的key表示sockfd， value表示sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel*> ;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_;
};