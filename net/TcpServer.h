#pragma once
#include <string>
#include <unordered_map>

#include "base/noncopyable.h"
#include "base/Callback.h"
#include "net/Channel.h"
#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "net/TcpConnection.h"

// Tcp传输层服务器类，只需要向应用层提供onMessage和onConnection两个回调函数
class TcpServer : noncopyable {
 public:
  TcpServer(unsigned short srcPort, EventLoop* loop);
  ~TcpServer(){};
  // 这个是针对新连接的回调函数
  void handleNewConn();
  // 针对TcpConnection的可读回调函数
  void handleMessage();

  void setConnCallback(const connCallback& cb);
  void setMessageCallback(const messageCallback& cb);
  void start();

  void removeConnection(const TcpConnectionPtr& conn);
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

 private:
  std::string srcAddr_ = "127.0.0.1";
  unsigned short srcPort_;
  int listenFd_;
  EventLoop* loop_;
  std::unique_ptr<EventLoopThreadPool> pool_;
  Channel* acceptChannel_;
  bool started_;
  std::unordered_map<std::string, TcpConnectionPtr> connections_;
  int count_ = 0;

  // 设定TcpConnection的回调函数
  connCallback connCallback_;
  messageCallback messageCallback_;

  // 服务器支持的最大连接数
  static const int MAXFDS = 100000;
};
