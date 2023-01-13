#include <mymuduo/TcpClient.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

class EchoClient
{
public:
    EchoClient(EventLoop* loop,
                const InetAddress &addr,
                const std::string &name)
            : client_(loop, addr, name)
            , loop_(loop)
    {
        
    }

    void start()
    {
        cli
    }
private:
    TcpClient client_;
    EventLoop* loop_;
};