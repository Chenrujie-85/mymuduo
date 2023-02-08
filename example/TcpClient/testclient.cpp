#include <mymuduo/TcpClient.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>
#include <iostream>
using namespace std;

class EchoClient
{
public:
    EchoClient(EventLoop* loop,
                const InetAddress &addr,
                const std::string &name)
            : client_(loop, addr, name)
            , loop_(loop)
    {
        client_.setConnectionCallback(
            std::bind(&EchoClient::onConnection, this, std::placeholders::_1)
        );

        client_.setMessageCallback(
            std::bind(&EchoClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        client_.setWriteCompleteCallback(
            std::bind(&EchoClient::onWriteComplete, this)
        ); 
    }

    void start()
    {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("CONN UP : %s", conn->localAddress().toIpPort().c_str());
            conn->send("hello \n");
        }
        else
        {
            LOG_INFO("CONN DOWN : %s", conn->localAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        cout << msg <<endl;
        // if(msg.substr(0, msg.size()-2) == "what do you want to do")
        // {
        //     string input;
        //     cin >> input;
        //     conn->send(input + " \n");
        // }
    }

    void onWriteComplete()
    {
        std::cout << "write complete" << std::endl;
    }

    TcpClient client_;
    EventLoop* loop_;
};

int main()
{
    EventLoop loop;
    InetAddress serveraddr(8000, "127.0.0.1");
    EchoClient client(&loop, serveraddr, "EchoClient");
    client.start();
    loop.loop();
    return 0;
}