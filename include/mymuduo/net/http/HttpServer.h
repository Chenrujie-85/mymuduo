#pragma once

#include <mymuduo/TcpServer.h>
#include <mymuduo/noncopyable.h>
#include <functional>

class HttpRequest;
class HttpResponse;

class HttpServer : noncopyable
{
public:
    using HttpCallback = std::function<void (const HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop* loop,
                const InetAddress& listenAddr,
                const std::string& name,
                TcpServer::Option option = TcpServer::kNoReusePort);
    
    ~HttpServer();

    EventLoop* getLoop() const {    return server_.getLoop();   }
    // 设置http请求的回调函数
    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }

    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    void start();

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,
                    Buffer* buf,
                    Timestamp receiveTime);
    // 在onMessage中调用，并调用用户注册的httpCallback_函数，对请求进行具体的处理                
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);

    TcpServer server_;
    HttpCallback httpCallback_;
};