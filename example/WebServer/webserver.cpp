#include <mymuduo/http/HttpServer.h>
#include <functional>

class WebServer
{
public:
    WebServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const std::string& name)
            : httpserver_(loop, listenAddr, name),
              loop_(loop)
    {
        //httpserver_.setHttpCallback();
        httpserver_.setThreadNum(4);
        httpserver_.setHttpCallback(std::bind(&WebServer::onResponse, this));
    }


    void start()
    {
        httpserver_.start();
    }
private:
    void onResponse()
    {
        
    }

    HttpServer httpserver_;
    EventLoop *loop_;
};

int main()
{
    EventLoop loop;
    InetAddress serveraddr(8000, "192.168.131.129");
    WebServer webServer(&loop, serveraddr, "WebServer-01");
    webServer.start();
    loop.loop();
    return 0;
}