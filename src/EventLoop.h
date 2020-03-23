#pragma once
#include <functional>
#include <vector>
#include <memory>
#include "../base/noncopyable.h"
#include "Channel.h"
#include "Epoll.h"

class EventLoop : noncopyable {
public:
  typedef std::function<void()> Functor;
  EventLoop();
  ~EventLoop() {};

  // 始终处于loop的状态，因此需要定义一些可以插入loop的函数
  void loop();
  void quit();

  // 向外暴露Epoll的接口，因为epoll实例属于EventLoop
  void removefromPoller(SP_Channel channel) {
    poller_->epoll_del(channel);
  }
  void updatePoller(SP_Channel channel, int timeout = 0) {
    poller_->epoll_mod(channel, timeout);
  }
  void addToPoller(SP_Channel channel, int timeout = 0) {
    poller_->epoll_add(channel, timeout);
  }
private:
// 状态变量
  bool looping_;  // 表示是否在循环中
  bool quit_;
  bool eventHandling_;
  bool callingPendingFunctors_;

  std::shared_ptr<Epoll> poller_;
  std::vector<Functor> pendingFunctors_;
};