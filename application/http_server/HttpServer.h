#pragma once
#include <fstream>
#include <functional>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "HttpRequest.h"
#include "HttpRespond.h"
#include "base/Callback.h"
#include "base/Logger.h"
#include "base/noncopyable.h"
#include "base/utility.h"
#include "net/EventLoop.h"
#include "net/TcpServer.h"

class HttpServer : noncopyable {
 public:
  using HttpHandler = std::function<void(const HttpRequest&, HttpRespond&)>;
  HttpServer(unsigned short port, EventLoop* loop,
             const std::string& host = "127.0.0.1")
      : host_(host), port_(port), tcpServer_(port, loop) {}
  ~HttpServer() = default;

  void start();

  // Get请求可能是请求文件，也可能是请求数据库中的某些信息
  HttpServer& Get(const std::string& pattern, const HttpHandler& httpHandler);

  // 设置URL相关请求对应的锚点
  // 例如针对file文件夹中example文件的请求file/example，我们可以将file前缀锚定到服务器的static文件夹，而后file请求都会锚定到static文件夹
  bool setMountPoint(const std::string& mp, const std::string& dir);

  // TODO 提供定时回调接口

  void setMessageCallback();
  void setConnCallback();
  void setWriteCompleteCallback();

  bool handleForFile(const HttpRequest& req, HttpRespond& res);
  // TODO 暂时默认不会出错
  void readFile(const std::string& path, std::string& body);
  bool routingHandler(const HttpRequest& req, HttpRespond& res);
  bool dispatchRequest(
      const HttpRequest& req, HttpRespond& res,
      std::vector<std::pair<std::regex, HttpHandler>>& handlers);
  bool helloTest(const HttpRequest& req, HttpRespond& res);

 private:
  std::string host_;
  unsigned short port_;
  TcpServer tcpServer_;
  std::vector<std::pair<std::regex, HttpHandler>> getHandlers_;
  std::string staticFilePathPrefix_ =
      "/home/lab515/WebServer/MySimpleWebServer/static/";
  std::vector<std::pair<std::string, std::string>> mountToDir_;
  // 请求到达的路由处理函数
};
