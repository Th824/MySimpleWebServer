# pragma once

# include <thread>
# include <condition_variable>
# include <mutex>
# include "base/noncopyable.h"
# include "EventLoop.h"

// 该类是对每个subReactor线程的封装，subReactor线程主要是运行一个EventLoop，每个EventLoop包含一个Epoll，将需要监听的
// 文件描述符加入到Epoll中，在loop开启以后每当有时间产生时，处理事件，同时，需要定义方法使得主Reactor能够将新的连接分配
// 到正在loop的线程
class EventLoopThread : noncopyable {
public:
  EventLoopThread();
  ~EventLoopThread();
  EventLoop* startLoop();
  
private:
  void threadFunc();
  EventLoop* loop_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
};