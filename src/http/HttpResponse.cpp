#include "HttpResponse.h"
#include "HttpRequest.h"
#include "../Buffer.h"
#include <string.h>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void HttpResponse::appendToBuffer(Buffer* output) const
{
    char buf[32];
    //响应行
    snprintf(buf, sizeof buf, "HTTP/1.1 %d %s", statusCode_, statusMessage_.c_str());
    output->append(buf);
    output->append("\r\n");
    //响应头部
    if(closeConnection_)
    {
        output->append("Connection: close\r\n");
    }
    else
    {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }
    //响应头部
    for(const auto& header : headers_)
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");
    output->append(body_);


    //std::cout<<output->retrieveAllAsString()<<std::endl;
}

void HttpResponse::addsponse(const HttpRequest req, const std::string& url)
{
    std::string real_file_url = url;
    struct stat real_file_stat;
    if(req.path() != "/")
        real_file_url += req.path();
    else
        real_file_url += req.path() + "log.html";
    if(stat(real_file_url.c_str(), &real_file_stat) < 0)
    {
        this->setStatusCode(700);
    }
    if(!(real_file_stat.st_mode & S_IROTH)){
        this->setStatusCode(403);
    }
    if(S_ISDIR(real_file_stat.st_mode)){
        this->setStatusCode(400);
    }        
    int fd = open(real_file_url.c_str(), O_RDONLY);
    char* my_file_address = (char*)mmap(0, real_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    this->setStatusCode(200);
    this->setStatusMessage("OK");
    this->setBody(my_file_address);
}