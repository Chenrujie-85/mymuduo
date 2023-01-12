#include "SocketsOps.h"
#include <errno.h>
#include "Logger.h"
#include <sys/uio.h>
#include <unistd.h>
#include <strings.h> 
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>

int sockets::createNonblockint()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

int  sockets::connect(int sockfd, const sockaddr* addr)
{
    return ::connect(sockfd, addr, sizeof(sockaddr_in));
}

void sockets::bindOrDie(int sockfd, const sockaddr* addr)
{
    if(0 != ::bind(sockfd, addr, sizeof(sockaddr_in)))
    {
        LOG_FATAL("sockets::bindOrDie %d\n", sockfd);
    }
}

void sockets::listenOrDie(int sockfd)
{
    if(0 != ::listen(sockfd, 1024))
    {
        LOG_FATAL("listen sockfd fail %d\n", sockfd);
    }
}

int  sockets::accept(int sockfd, sockaddr_in* addr)
{
    socklen_t addrlen = sizeof(&addr);
    int connfd = ::accept4(sockfd, (sockaddr*)&addr, &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (connfd < 0)
    {
        int savedErrno = errno;
        LOG_INFO("Socket::accept, sockfd = %d \n", connfd);
        switch (savedErrno)
        {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                LOG_ERROR("%s:%s:%d unexpected error of ::accept:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
                break;
            default:
                LOG_ERROR("%s:%s:%d unknown error of ::accept :%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
                break;
        }
    }
    return connfd;
}
ssize_t sockets::read(int sockfd, void *buf, size_t count)
{
    return ::read(sockfd, buf, count);
}

ssize_t sockets::readv(int sockfd, const iovec *iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void *buf, size_t count)
{
    return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd)
{
    if (::close(sockfd) < 0)
    {
        LOG_ERROR("sockets::close, sockfd = %d \n", sockfd);
    }
}

void sockets::shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG_ERROR("sockets::shutdownWrite, sockfd = %d \n", sockfd);
    }
}

void sockets::toIpPort(char* buf, size_t size,
            const sockaddr* addr)
{
    const sockaddr_in* addr_ = static_cast<const sockaddr_in*>(static_cast<const void*>(addr));;
    ::inet_ntop(AF_INET, &addr_->sin_addr, buf, static_cast<socklen_t>(size));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_->sin_port);
    snprintf(buf+end, size-end, ":%u", port);
}

void sockets::fromIpPort(const char* ip, uint16_t port,
                sockaddr_in* addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
        LOG_ERROR("sockets::fromIpPort");
    }
}


int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)//获取套接字当前的状态值
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

sockaddr_in sockets::getLocalAddr(int sockfd)
{
    sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, (sockaddr*)(&localaddr), &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    return localaddr;
}

sockaddr_in sockets::getPeerAddr(int sockfd)
{
    sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, (sockaddr*)(&peeraddr), &addrlen) < 0)
    {
        LOG_ERROR("sockets::peerLocalAddr");
    }
    return peeraddr;
}

bool sockets::isSelfConnect(int sockfd)
{
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);

    const struct sockaddr_in* laddr = reinterpret_cast<sockaddr_in*>(&localaddr);
    const struct sockaddr_in* raddr = reinterpret_cast<sockaddr_in*>(&peeraddr);
    return laddr->sin_port == raddr->sin_port
        && laddr->sin_addr.s_addr == raddr->sin_addr.s_addr;
}