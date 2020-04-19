# include "EventLoopThread.h"
# include <functional>
# include <assert.h>

EventLoopThread::EventLoopThread() : loop_(nullptr) {}

EventLoopThread::~EventLoopThread() {
  if (loop_ != nullptr) {
    loop_->quit();
    // 析构函数调用join
    thread_.join();
  }
}

EventLoop* EventLoopThread::startLoop() {
  // 最好能对thread做一个包装，使得thread不必在初始化时启动
  thread_ = std::thread(std::bind(&EventLoopThread::threadFunc, this));
  {
    std::unique_lock<std::mutex> lck(mutex_);
    // 使用while避免虚假唤醒
    while (loop_ == nullptr) {
      cond_.wait(lck);
    }
  }
  return loop_;
}

void EventLoopThread::threadFunc() {
  EventLoop loop;
  {
    std::unique_lock<std::mutex> lck(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }

  loop.loop();
  // 线程离开时，将loop_置为空，而loop作为一个栈上对象会自动调用析构函数
  loop_ = nullptr;
}