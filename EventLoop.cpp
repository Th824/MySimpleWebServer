# include "EventLoop.h"
# include <assert.h>
# include <iostream>
# include <sys/eventfd.h>
# include <sys/epoll.h>
# include <functional>
# include "base/utility.h"

// 通过eventfd来实现wakeup，将wakeupFd添加到epoll中，如果需要唤醒，则往
// wakeupFd中写入数据，这样epoll就能从epoll_wait中返回。
int createEventfd() {
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    std::cout << "Failed to create event fd" << std::endl;
    abort();
  }
  return evtfd;
}

EventLoop::EventLoop() : 
  looping_(false),
  quit_(false),
  poller_(new Epoll()),
  wakeupFd_(createEventfd()),
  callingPendingFunctors_(false),
  pwakeupChannel_(new Channel(this, wakeupFd_)),
  threadId_(std::this_thread::get_id()) {
    // 设置pwakeupChannel
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_->setReadHandler(std::bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(std::bind(&EventLoop::handleConn, this));
    pwakeupChannel_->addToEpoll();
}

void EventLoop::loop() {
  assert(!looping_);
  assert(isInLoopThread());
  looping_ = true;
  quit_ = false;
  std::vector<Channel*> ret;
  while (!quit_) {
    ret.clear();
    // 在这里阻塞直到有事件产生
    ret = poller_->poll();
    std::cout << ret.size() << " events happen" << std::endl;
    // 调用有事件产生的Channel的回调函数
    for (auto &it : ret) {
      it->handleEvents();
    }
    doPendingFunctors();
    // 这里应该调用poller处理超时的Channel
    // poller_->handleExpired();
  }
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  // 设置退出标志后，可能还阻塞在epoll_wait中，此时通过wakeup唤醒
  if (!isInLoopThread()) {
    wakeup();
  }
}

bool EventLoop::isInLoopThread() const {
  return threadId_ == std::this_thread::get_id();
}

void EventLoop::runInLoop(Functor&& cb) {
  // 如果由IO线程调用，那么直接执行函数，否则将其加入到回调队列中等待执行
  if (isInLoopThread()) {
    std::cout << "here" << std::endl;
    cb();
  } else {
    std::cout << "here2" << std::endl;
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor &&cb) {
  {
    std::lock_guard<std::mutex> lck(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }

  // 当由其他进程添加回调函数或者IO线程调用但是IO线程进行到在执行回调函数的时候，此时如果
  // 添加了回调函数，则需要通过wakeup来使得在下一次循环时epoll_wait能够返回，使添加的回调
  // 函数能够及时被执行
  if (!isInLoopThread() || callingPendingFunctors_) wakeup();
}

void EventLoop::assertInLoopThread() {
  assert(isInLoopThread());
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  // 直接使用swap操作取出pendingFunctors_中的回调函数，缩小了临界区大小
  {
    std::lock_guard<std::mutex> lck(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (auto functor : functors) functor();
  callingPendingFunctors_ = false;
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  // 往wakeupFd中写入数据
  ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof(one));
  if (n != sizeof one) {
    std::cout << "EventLoop::wakeup() writes " << n << " bytes instead of 8" << std::endl;
  }
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  // 从wakeupFd中读出数据
  ssize_t n = readn(wakeupFd_, &one, sizeof(one));
  if (n != sizeof one) {
    std::cout << "EventLoop::handleRead() reads " << n << " bytes instead of 8" << std::endl;
  }
  // 没有设置oneshot的标志的话不用重新设置epoll事件标志
}

void EventLoop::handleConn() {
  pwakeupChannel_->update();
}