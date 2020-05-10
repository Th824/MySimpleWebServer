#include "HttpServer.h"
#include "base/Logger.h"
#include "net/EventLoop.h"
#include <unistd.h>
#include <iostream>

bool Logger::outputToConsole = true;

int main() {
  unsigned short port = 30000;
  EventLoop *loop = new EventLoop();
  char buf[500];
  getcwd(buf, sizeof(buf));
  std::string basePath(buf);
  basePath += "/../static/blog";
  
  // 主进程也是一个循环，且只监听了一个事件，就是连接事件
  HttpServer httpServer(port, loop);
  std::cout << basePath << std::endl;
  // 如果仅仅设置了mountPoint的话，就是一个静态文件服务器
  httpServer.setMountPoint("/", basePath);
  // httpServer.setFileRequestHandler([](const HttpRequest& req, HttpRespond& res) {
  //   std::string filePath = res.getRequestFilePath();
  //   res.setBody();
  // });
  httpServer.start();
  return 0;
}