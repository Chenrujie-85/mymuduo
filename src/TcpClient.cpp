#include "TcpClient.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Connector.h"
#include "SocketsOps.h"
#include <stdio.h>
#include <functional>
#include <string>
#include "Callbacks.h"

static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop in null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

namespace detail
{
void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
}

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr, const std::string& nameArg)
    : loop_(CheckLoopNotNull(loop))
    , connector_(new Connector(loop, serverAddr))//创建一个connector,与Tcpclient一对一
    , name_(nameArg)

    , retry_(false)//失败时是否重连
    , connect_(true)
    , nextConnId_(1)//对成功连接进行计数
{
    connector_->setNewConnectionCallback(
        std::bind(&TcpClient::newConnection, this, std::placeholders::_1)
    );
    LOG_INFO("TcpClient::TcpClient connector");
}

TcpClient::~TcpClient()
{
    LOG_INFO("TcpClient::connect [%s] down \n", name_.c_str());
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn)
    {
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
        loop_->runInLoop(
            std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique)
        {
            conn->forceClose();
        }
    }
    // else
    // {
    //     connector_->stop();
    //     loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    // }
}

void TcpClient::connect()
{
    LOG_INFO("TcpClient::connect [%s] - connecting to %s \n", name_.c_str(), connector_->serverAddress().toIpPort().c_str());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (connection_)
        {
        connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

//连接成功时调用该函数
void TcpClient::newConnection(int sockfd)
{
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));
    conn->setConnectionCallback(connectionCallback_); //默认打印日志
    conn->setMessageCallback(messageCallback_);	//默认重置缓冲区
    conn->setWriteCompleteCallback(writeCompleteCallback_); //默认不存在
    conn->setCloseCallback(
        std::bind(&TcpClient::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe //默认
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_ = conn; //每个Tcpclient对应一个TcpConnection
    }
    conn->connectEstablished(); //加入IO multiplexing //注册可读事件
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_.reset();
    }
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if(retry_ && connect_)
    {
        LOG_INFO("TcpClient::connect [%s] - Reconnecting to %s \n", name_.c_str(), connector_->serverAddress().toIpPort().c_str());
        connector_->restart();
    }
}

// TcpConnectionPtr TcpClient::connection() const
// {
//     std::unique_lock<std::mutex> lock(mutex_);
//     return connection_;
// }