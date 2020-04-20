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

class HttpServer : noncopyable {
public:
  using HttpHandler = std::function<void(const HttpRequest&, HttpRespond&)>;
  HttpServer(unsigned short listenedPort, EventLoop *loop) : tcpServer_(listenedPort, loop){
    
  }

  void start() {
    tcpServer_.start();
  }

  HttpServer& Get(const std::string& pattern, const HttpHandler &httpHandler) {
    // 使用pattern创建regex对象，并将其加入getHandlers_中
    getHandlers_.emplace_back(std::regex(pattern), httpHandler);
    return *this;
  }

  // TODO 提供定时回调接口

  void setMessageCallback() {
    tcpServer_.setMessageCallback(messageCallback_);
  }

  void setConnCallback() {
    tcpServer_.setConnCallback(connCallback_);
  }

  bool routingHandler(const HttpRequest& req, HttpRespond& res) {
    std::string path = req.path();
    std::string method = req.method();
    if (method == "GET") {
      dispatchRequest(req, res, getHandlers_);
    }
  }

  void dispatchRequest(const HttpRequest& req, HttpRespond& res, std::vector<HttpRequest> &handlers) {
    try {
      for (const auto &x : handlers) {
        const auto &pattern = x.first;
        const auto &handler = x.second;

        if (std::regex_match(req.path, req.matches, pattern)) { // todo: 在计算线程进行运算
          // handler中可能会涉及到计算复杂度高的操作，或者需要IO的操作
          handler(req, res);
          return true;
        }
      }
    } catch (const std::exception &ex) {
      res.status = 500;
      res.set_header("EXCEPTION_WHAT", ex.what());
    } catch (...) {
      res.status = 500;
      res.set_header("EXCEPTION_WHAT", "UNKNOWN");
    }
    return false;
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
      routing
    }
  }

  void connCallback_(const TcpConnectionPtr &conn) {
    
  }
  // 请求到达的路由处理函数
  
};
