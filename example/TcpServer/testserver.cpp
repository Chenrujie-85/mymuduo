#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>

#include <string>
#include <functional>

using namespace std;

// 使用muduo开发回显服务器
class EchoServer
{
public:
    EchoServer(EventLoop* loop,
                const InetAddress &addr,
                const std::string &name)
            : server_(loop, addr, name)
            , loop_(loop)
    {
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1)
        );

        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
        );

        server_.setWriteCompleteCallback(
            std::bind(&EchoServer::onWriteComplete, this, std::placeholders::_1)
        );

        server_.setThreadNum(3);
    }

    void start()
    {
        server_.start();
    }
private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            LOG_INFO("CONN UP : %s", conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("CONN DOWN : %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        cout << msg <<endl;
        if(msg.substr(0, msg.size()-2) == "hello")
        {
            //conn->send("hello \n");
            conn->send("what do you want to do \n");
        }
        else if(msg.substr(0, msg.size()-2) == "bye")
        {
            conn->send("good bye \n");
            conn->shutdown(); //写端 EPOLLHUP=>closeCallback_
        }
        else
        {
            conn->send("what do you want to do \n");
        }
    }

    void onWriteComplete(const TcpConnectionPtr& conn)
    {
        cout<<"writecomplete"<<endl;
    }
    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress listenAddr(8000);
    EchoServer server(&loop, listenAddr, "EchoServer-01");
    server.start();
    loop.loop();
    return 0;
}
