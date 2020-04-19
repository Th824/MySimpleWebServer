#pragma once
# include <unordered_map>
# include <string>
# include "base/noncopyable.h"
# include "net/EventLoop.h"
# include "net/Channel.h"
# include "net/EventLoopThreadPool.h"
# include "net/TcpConnection.h"
# include "HttpRequest.h"
# include "HttpRespond.h"

// 服务器类
class Server : noncopyable {
private:
  using messageCallback = std::function<void (const TcpConnectionPtr&, Buffer*)>;
  using connCallback = std::function<void (const TcpConnectionPtr&)>;
public:
  Server(unsigned short srcPort, EventLoop *loop);
  ~Server(){};
  // 这个是针对新连接的回调函数
  void handleNewConn();
  // 针对TcpConnection的可读回调函数
  void handleMessage();

  void setConnCallback(const connCallback& cb) {connCallback_ = cb;}
  void setMessageCallback(const messageCallback& cb) {messageCallback_ = cb;}
  void start();

  void removeConnection(const TcpConnectionPtr& conn);
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

private:
  std::string srcAddr_ = "127.0.0.1";
  unsigned short srcPort_;
  int listenFd_;
  EventLoop *loop_;
  std::unique_ptr<EventLoopThreadPool> pool_;
  Channel* acceptChannel_;
  bool started_;
  std::unordered_map<std::string, TcpConnectionPtr> connections_;
  int count_ = 0;

  // 设定TcpConnection的回调函数
  connCallback connCallback_;
  messageCallback messageCallback_;

private:
  static const int MAXFDS = 100000;
};
