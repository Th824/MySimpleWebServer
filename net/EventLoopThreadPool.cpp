#include "EventLoopThreadPool.h"
#include "base/Logger.h"
#include <assert.h>
#include <iostream>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads)
    : baseLoop_(baseLoop), started_(false), numThreads_(numThreads), next_(0) {
  if (numThreads_ <= 0) {
    LOG << "numThreads_ <= 0";
  }
  LOG << "Initial " << numThreads_ << " thread pool successfully";
}

EventLoopThreadPool::~EventLoopThreadPool() {
  // TODO join所有的线程，删除对应的EventLoop
  // LOG << "delete thread pool";
}

void EventLoopThreadPool::start() {
  // TODO
  assert(!started_);
  baseLoop_->assertInLoopThread();
  started_ = true;
  for (int i = 0; i < numThreads_; i++) {
    std::shared_ptr<EventLoopThread> t(new EventLoopThread());
    threads_.push_back(t);
    loops_.push_back(t->startLoop());
  }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
  // TODO
  baseLoop_->assertInLoopThread();
  assert(started_);
  // 如果没有subReactor，则将其添加到mainReactor中
  EventLoop* loop = baseLoop_;
  if (!loops_.empty()) {
    loop = loops_[next_];
    next_ = (next_ + 1) % numThreads_;
  }
  return loop;
}