#include "HttpServer.h"
#include "net/EventLoop.h"
#include "base/Logger.h"

bool Logger::outputToConsole = true;

int main() {
  unsigned short port = 30000;
  EventLoop *loop = new EventLoop();
  // 主进程也是一个循环，且只监听了一个事件，就是连接事件
  HttpServer httpServer(port, loop);
  httpServer.setMountPoint("/", "test");
  httpServer.start();
  return 0;
}