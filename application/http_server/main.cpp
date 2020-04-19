// # include "Server.h"
# include "net/EventLoop.h"
# include "Server.h"

int main() {
  unsigned short port = 30000;
  EventLoop *loop = new EventLoop();
  // 主进程也是一个循环，且只监听了一个事件，就是连接事件
  Server server(port, loop);
  server.start();
  loop->loop();
  return 0;
}