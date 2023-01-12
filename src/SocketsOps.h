#pragma once

#include <arpa/inet.h>

namespace sockets
{
    int createNonblockint();

    int  connect(int sockfd, const sockaddr* addr);
    void bindOrDie(int sockfd, const sockaddr* addr);
    void listenOrDie(int sockfd);
    int  accept(int sockfd, sockaddr_in* addr);
    ssize_t read(int sockfd, void *buf, size_t count);
    ssize_t readv(int sockfd, const iovec *iov, int iovcnt);
    ssize_t write(int sockfd, const void *buf, size_t count);

    void close(int sockfd);
    void shutdownWrite(int sockfd);

    void toIpPort(char* buf, size_t size,
                const sockaddr* addr);

    void fromIpPort(const char* ip, uint16_t port,
                    sockaddr_in* addr);

    int getSocketError(int sockfd);

    sockaddr_in getLocalAddr(int sockfd);
    sockaddr_in getPeerAddr(int sockfd);
    bool isSelfConnect(int sockfd);
}