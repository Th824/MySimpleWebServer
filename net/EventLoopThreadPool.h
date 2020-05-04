# pragma once

# include "EventLoop.h"
# include "EventLoopThread.h"
# include <thread>
# include <vector>
# include "base/noncopyable.h"

class EventLoopThreadPool : noncopyable {
public:
  EventLoopThreadPool(EventLoop* baseLoop, int numThreads = 4);
  ~EventLoopThreadPool();
  EventLoop* getNextLoop();
  void start();

private:
  EventLoop* baseLoop_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::shared_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};