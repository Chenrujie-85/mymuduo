#pragma once

#include "TcpConnection.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoop.h"
#include <memory>
#include <string>
#include "Callbacks.h"
#include <atomic>
#include <mutex>

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& nameArg);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    //TcpConnectionPtr connection() const;

    EventLoop* getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry() { retry_ = true; }

    const std::string& name() const { return name_; }

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

private:
    void newConnection(int sockfd);
    void removeConnection(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr connector_;
    const std::string name_;
    
    ConnectionCallback connectionCallback_;//默认打印日志
    MessageCallback messageCallback_;//默认重置缓冲区
    WriteCompleteCallback writeCompleteCallback_;

    std::atomic_bool retry_;//连接断开后是否重连
    std::atomic_bool connect_;
    int nextConnId_;
    std::mutex mutex_;
    TcpConnectionPtr connection_;
};