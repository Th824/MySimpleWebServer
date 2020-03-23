#pragma once
# include "../base/noncopyable.h"
# include "EventLoop.h"
# include "Channel.h"

// 服务器类
class Server : noncopyable {
private:
  static const int MAXFDS = 100000;

private:
  int port_;
  int listenFd_;
  EventLoop *loop_;
  SP_Channel acceptChannel_;
  bool started_;

public:
  Server(int port, EventLoop *loop);
  ~Server() {};
  void handleNewConn();
  void handleThisConn();
  void start();
};
