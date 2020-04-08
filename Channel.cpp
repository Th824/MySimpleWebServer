# include "Channel.h"
# include "EventLoop.h"

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), lastEvents_(0) {}

/*
这里的设计有点奇怪，handleConn是每次有事件产生都会调用的回调函数，
主要用来对epoll中的文件描述符重新设置监听事件，因为ONE_SHOT的原因
每一次触发事件后都需要重新设置标志。
*/
void Channel::handleConn() {
  if (connHandler_) connHandler_();
}

void Channel::handleRead() {
  if (readHandler_) readHandler_();
}

void Channel::handleWrite() {
  if (writeHandler_ ) writeHandler_();
}

// void Channel::handleError(int fd, int err_num, std::string short_msg) {
  
// }