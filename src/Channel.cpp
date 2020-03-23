# include "Channel.h"
# include "EventLoop.h"

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), lastEvents_(0) {}

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