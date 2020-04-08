#pragma once
# include "base/noncopyable.h"
# include "EventLoop.h"
# include "Channel.h"
# include "EventLoopThreadPool.h"

// 服务器类
class Server : noncopyable {
public:
  Server(int port, EventLoop *loop);
  ~Server(){};
  void handleNewConn();
  void handleThisConn();
  void start();

private:
  int port_;
  int listenFd_;
  EventLoop *loop_;
  std::unique_ptr<EventLoopThreadPool> pool_;
  SP_Channel acceptChannel_;
  bool started_;

private:
  static const int MAXFDS = 100000;
};
