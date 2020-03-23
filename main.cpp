# include "src/Server.h"
# include "src/EventLoop.h"

int main() {
  int port = 30000;
  EventLoop *loop = new EventLoop();
  Server server(port, loop);
  server.start();
  loop->loop();
  return 0;
}