# pragma once
# include "net/TcpServer.h"
# include "net/EventLoop.h"
# include "base/noncopyable.h"
# include "base/Callback.h"
# include "HttpRequest.h"
# include "HttpRespond.h"
# include <functional>
# include <string>
# include <vector>
# include <regex>

class HttpServer: noncopyable {
  using HttpHandler = std::function<void(const HttpRequest&, HttpRespond&)>;

  HttpServer(unsigned short listenedPort, EventLoop *loop) : tcpServer_(listenedPort, loop){
    // tcpServer_.setMessageCallback(std::bind(&HttpServer::messageCallback_, this));
    tcpServer_.setMessageCallback(messageCallback_);
    tcpServer_.setConnCallback(HttpServer::connCallback_);
  }
  
  HttpServer& Get(const std::string& pattern, const HttpHandler &httpHandler) {
    // 使用pattern创建regex对象，并将其加入getHandlers_中
    getHandlers_.emplace_back(std::regex(pattern), httpHandler);
    return *this;
  }

  void start() {
    tcpServer_.start();
  }

private:
  TcpServer tcpServer_;
  std::vector<std::pair<std::regex, HttpHandler>> getHandlers_;
 
  void messageCallback_(const TcpConnectionPtr &conn, Buffer *buf) {
    std::shared_ptr<HttpRequest> httpRequest = conn->httpRequest();
    httpRequest->parseRequest(buf);
    // 在获取完全的request信息后，调用注册的路由函数
    if (httpRequest->state() == "Done") {
      // HttpRespond httpRespond;
      // httpRespond.setStateCode("200");
      // httpRespond.setBody("hello world");
      // httpRespond.setContentLength(11);
      // conn->send(httpRespond.generateRespond());
      // httpRequest->reset(); 

      // invoke
    }
  }

  // void connCallback_(const TcpConnectionPtr &conn) {
  //   // todo: 当连接建立 / 断开的时候会被调用
  // }
};