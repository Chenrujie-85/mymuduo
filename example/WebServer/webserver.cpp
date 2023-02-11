#include <mymuduo/http/HttpServer.h>
#include <mymuduo/http/HttpResponse.h>
#include <mymuduo/http/HttpRequest.h>
#include <functional>

std::string url = "/home/nagomi/mymuduo/example/WebServer/file";

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
        httpserver_.setHttpCallback(std::bind(&WebServer::onResponse, this, 
                                    std::placeholders::_1, std::placeholders::_2));
    }


    void start()
    {
        httpserver_.start();
    }
private:
    void onResponse(const HttpRequest& req, HttpResponse* response)
    {
        response->addsponse(req, url);
        //文件映射
        //addheader
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