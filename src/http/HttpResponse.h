#pragma once

#include <map>

class Buffer;
class HttpRequest;

class HttpResponse
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k403ForBidden = 403,
        k404NotFound = 404,
        k700NoResource = 700,
    };

    explicit HttpResponse(bool close)
    : statusCode_(kUnknown),
      closeConnection_(close)
    {
    }

    void setStatusCode(int code)
    {
        setStatusCode(HttpStatusCode(code));
    }

    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    void setStatusMessage(const std::string& message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }

    void setContentType(const std::string& contentType)
    {   addHeader("content-Type", contentType); }

    void addHeader(const std::string& key, const std::string& value)
    {   headers_[key] = value;  }

    void setBody(const std::string& body)
    {   body_ = body;   }

    void appendToBuffer(Buffer* output) const;

    void addsponse(const HttpRequest req, const std::string& real_file_url);
private:
    std::map<std::string, std::string> headers_;
    HttpStatusCode statusCode_;
    std::string statusMessage_;
    bool closeConnection_;
    std::string body_;
};