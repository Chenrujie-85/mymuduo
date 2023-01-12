#include "Connector.h"
#include "Logger.h"
#include "EventLoop.h"
#include <errno.h>
#include "SocketsOps.h"

Connector::Connector(EventLoop* loop, const InetAddress& serverAddress)
    : loop_(loop)
    , serverAddr_(serverAddress)
    , connect_(false)
    , state_(kDisconnected)
    , retryDelayMs_(kInitRetryDelayMs)
{
    LOG_INFO("get a conn, ipPort = %s \n", serverAddr_.toIpPort().c_str());
}

Connector::~Connector()
{
    LOG_DEBUG("down a conn, ipPort = %s \n", serverAddr_.toIpPort().c_str());
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    if(connect_)
    {
        connect();
    }
    else
    {
       LOG_DEBUG("do not conn \n"); 
    }
}

void Connector::stop()
{
    connect_ = false;
    loop_->runInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop()
{
    if(state_ = kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNonblockint();
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr_const());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS: //正在连接
        case EINTR: //当阻塞于某个慢系统调用的一个进程捕获某个信号且相应信号处理函数返回时，该系统调用可能返回一个EINTR错误。
        case EISCONN: //连接成功
            connecting(sockfd);
            break;

        case EAGAIN: //临时端口(ephemeral port)不足  
        case EADDRINUSE: //监听的端口已经被使用
        case EADDRNOTAVAIL: //配置的IP不对
        case ECONNREFUSED: //服务端在我们指定的端口没有进程等待与之连接
        case ENETUNREACH: //表示目标主机不可达 
            retry(sockfd);
            break;

        case EACCES: //没有权限
        case EPERM: //操作不被允许
        case EAFNOSUPPORT: //该系统不支持IPV6
        case EALREADY: //套接字非阻塞且进程已有待处理的连接
        case EBADF: //无效的文件描述符
        case EFAULT: //操作套接字时的一些参数无效
        case ENOTSOCK: //不是一个套接字
            LOG_ERROR("%s:%s:%d connect error in Connector::startInLoop:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
            sockets::close(sockfd);
            break;
        default:
            LOG_ERROR("%s:%s:%d Unexpected error in Connector::startInLoop:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }
}

void Connector::restart()
{
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting); //设置状态
    channel_.reset(new Channel(loop_, sockfd)); //把channel与fd相关联
    channel_->setWriteCallback(
        std::bind(&Connector::handleWrite, this)); 
    channel_->setErrorCallback(
        std::bind(&Connector::handleError, this)); 
    channel_->enableWriting(); //IO multiplexing关注可写事件 //socket变的可写时表示连接完成
}

void Connector::handleWrite()
{
    LOG_DEBUG("Connector::handleWrite state= %d \n", state_);

    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel(); //从poller中移除,并把channel置为空 socket出错无法恢复
        int err = sockets::getSocketError(sockfd); //可写不一定连接成功 使用getsockopt确认一下 功能为获取一个套接字的选项
        if (err)
        {
            LOG_ERROR("Connector::handleWrite - SO_ERROR = %d \n", err);
            retry(sockfd); //使用TimerQueue重新发起连接
        }
        // else if (sockets::isSelfConnect(sockfd)) //出现自连接的情况
        // {
        //     LOG_ERROR("Connector::handleWrite - Self connect \n");
        //     retry(sockfd);
        // }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd); //连接成功时执行回调 在TCPClient中设置
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else
    {
        LOG_ERROR("There had something wrong, state = %d \n", state_);
    }
}

void Connector::handleError()
{
    LOG_ERROR("Connector::handleError state= %d \n", state_);
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_DEBUG("SO_ERROR = %d \n", err);
        retry(sockfd); //出现错误的时候重新连接
    }
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

//重新尝试能否连接
void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_)//执行了start后执行这个
    {
        LOG_INFO("Connector::retry - Retry connecting to %s in milliseconds %d \n", serverAddr_.toIpPort().c_str(), retryDelayMs_);
    
        loop_->runAfter(retryDelayMs_/1000.0,
                        std::bind(&Connector::startInLoop, this));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}