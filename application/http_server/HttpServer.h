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
  // 设置为虚函数，后续方便继承扩展为Https服务器
  virtual ~HttpServer() = default;

  void start();

  // 函数的返回值设置为类型引用使得可以"连点"的调用方式
  // Get请求可能是请求文件，也可能是请求数据库中的某些信息
  HttpServer& Get(const std::string& pattern, const HttpHandler& httpHandler);
  HttpServer& Post(const std::string& pattern, const HttpHandler& httpHandler);
  HttpServer& Put(const std::string& pattern, const HttpHandler& httpHandler);
  HttpServer& Patch(const std::string& pattern, const HttpHandler& HttpHandler);
  HttpServer& Delete(const std::string& pattern,
                     const HttpHandler& HttpHandler);
  HttpServer& Option(const std::string& pattern,
                     const HttpHandler& HttpHandler);

  // 设置URL相关请求对应的锚点
  // 例如针对file文件夹中example文件的请求file/example，我们可以将file前缀锚定到服务器的static文件夹，而后file请求都会锚定到static文件夹
  bool setMountPoint(const std::string& mp, const std::string& dir);
  bool removeMountPoint(const std::string& mp);

  // TODO 提供定时回调接口，方便用户直接注册定时回调函数

  void setMessageCallback();
  void setConnCallback();
  void setWriteCompleteCallback();
  void setFileRequestHandler(HttpHandler fileRequestHandler);

  bool routing(const HttpRequest& req, HttpRespond& res);

  bool handleForFile(const HttpRequest& req, HttpRespond& res, bool isHead);
  bool handleForRangeRequest(const HttpRequest& req, HttpRespond& res);
  // TODO 暂时默认不会出错
  void readFile(const std::string& path, std::string& body);
  bool dispatchRequest(
      const HttpRequest& req, HttpRespond& res,
      std::vector<std::pair<std::regex, HttpHandler>>& handlers);
  bool helloTest(const HttpRequest& req, HttpRespond& res);

 private:
  std::string host_;
  unsigned short port_;
  TcpServer tcpServer_;
  std::vector<std::pair<std::regex, HttpHandler>> getHandlers_;
  std::vector<std::pair<std::regex, HttpHandler>> postHandlers_;
  std::vector<std::pair<std::regex, HttpHandler>> putHandlers_;
  std::vector<std::pair<std::regex, HttpHandler>> patchHandlers_;
  std::vector<std::pair<std::regex, HttpHandler>> deleteHandlers_;
  std::vector<std::pair<std::regex, HttpHandler>> optionHandlers_;
  HttpHandler fileRequestHandler_;
  std::vector<std::pair<std::string, std::string>> mountToDir_;
  // 请求到达的路由处理函数
};
