# include "EventLoop.h"
# include <assert.h>
# include <iostream>

EventLoop::EventLoop() : 
  looping_(false),
  poller_(new Epoll()) {
}

void EventLoop::loop() {
  assert(!looping_);
  looping_ = true;
  std::vector<SP_Channel> ret;
  while (true) {
    ret.clear();
    ret = poller_->poll();
    std::cout << ret.size() << " events happen" << std::endl;
    for (auto &it : ret) {
      it->handleEvents();
    }
  }
  looping_ = false;
}