#include "Channel.h"

#include "EventLoop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kDefaultEvent = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLET;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), lastEvents_(0) {}

/*
这里的设计有点奇怪，handleConn是每次有事件产生都会调用的回调函数，
主要用来对epoll中的文件描述符重新设置监听事件，因为ONE_SHOT的原因
每一次触发事件后都需要重新设置标志。
*/
// 根据发生的不同的数据类型执行不同的回调函数
void Channel::handleEvents() {
  // LOG << fd_ << " handleEvents " << revents_;
  events_ = 0;
  // 如果发生的事件是对端关闭且没有写入数据
  if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
  // if (revents_ & EPOLLHUP) {
    // LOG << fd_ << " close";
    events_ = 0;
    if (closeHandler_) closeHandler_();
    return;
  }
  if (revents_ & EPOLLERR) {
    // LOG << fd_ << " error happen";
    if (errorHandler_) errorHandler_();
    events_ = 0;
    return;
  }
  if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
    // LOG << fd_ << " can read";
    handleRead();
  }
  if (revents_ & EPOLLOUT) {
    // LOG << fd_ << " can write";
    handleWrite();
  }
  // handleConn();
}

void Channel::handleConn() {
  if (connHandler_) connHandler_();
}

void Channel::handleRead() {
  if (readHandler_) readHandler_();
}

void Channel::handleWrite() {
  if (writeHandler_) writeHandler_();
}

// void Channel::handleError(int fd, int err_num, std::string short_msg) {

// }

void Channel::update() { loop_->updatePoller(this); }

void Channel::addToEpoll() { loop_->addToPoller(this); }

// 从epoll中移出该channel，一般发生在连接销毁
void Channel::remove() { loop_->removefromPoller(this); }