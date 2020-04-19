#pragma once
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include "Channel.h"
#include "Epoll.h"
#include "base/noncopyable.h"

class EventLoop : noncopyable {
public:
  typedef std::function<void()> Functor;
  EventLoop();
  ~EventLoop() {};

  // 始终处于loop的状态，因此需要定义一些可以插入loop的函数
  void loop();
  void quit();

  bool isInLoopThread() const;
  void runInLoop(Functor &&cb);
  void queueInLoop(Functor &&cb);
  void assertInLoopThread();

  // 向外暴露Epoll的接口，因为epoll实例属于EventLoop
  void removefromPoller(Channel* channel) {
    poller_->epoll_del(channel);
  }
  void updatePoller(Channel* channel, int timeout = 0) {
    poller_->epoll_mod(channel, timeout);
  }
  void addToPoller(Channel* channel, int timeout = 0) {
    poller_->epoll_add(channel, timeout);
  }
private:
// 状态变量
  bool looping_;  // 表示是否在循环中
  bool quit_;
  bool eventHandling_;
  // 创建了EventLoop的线程就是IO线程
  const std::thread::id threadId_;
  std::mutex mutex_;

  // callingPendingFunctors_和wakeupFd_互相配合，实现回调函数的及时调用
  int wakeupFd_;
  bool callingPendingFunctors_;
  std::unique_ptr<Channel> pwakeupChannel_;
  std::unique_ptr<Epoll> poller_;
  std::vector<Functor> pendingFunctors_;

  void wakeup();
  void doPendingFunctors();
  // wakeupChannel的回调函数
  void handleRead();
  void handleConn();
};