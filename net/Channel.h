#pragma once
#include <poll.h>
#include <sys/epoll.h>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "base/noncopyable.h"

class EventLoop;

class Channel : noncopyable {
 private:
  const static int kNoneEvent;
  const static int kDefaultEvent;
  // 定义回调函数的形式
  typedef std::function<void()> Callback;
  EventLoop *loop_;  // 一个Channel所属的Eventloop
  int fd_;           // Channel关注的fd

  __uint32_t events_;   // Channel所关联的fd感兴趣的事件
  __uint32_t revents_;  // Channel所关联的fd上发生的事件
  __uint32_t lastEvents_;

  Callback readHandler_;
  Callback writeHandler_;
  Callback errorHandler_;
  Callback connHandler_;
  Callback closeHandler_;

 public:
  Channel(EventLoop *loop,
          int fd);  // Channel构造函数，应该在构造连接时由Connection类负责构造
  ~Channel(){};  // 析构函数，应该由拥有该Channel的连接类来调用

  int getFd() { return fd_; };
  void setFd(int fd) { fd_ = fd; };

  void setRevents(__uint32_t ev) { revents_ = ev; }
  void setEvents(__uint32_t ev) { events_ = ev; }
  void setDefaultEvents() { events_ = kDefaultEvent; }
  void setNoneEvents() { events_ = kNoneEvent; }
  // 原代码中返回引用
  __uint32_t getEvents() { return events_; }
  __uint32_t getLastEvents() { return lastEvents_; }

  bool EqualAndUpdateLastEvents() {
    bool res = (lastEvents_ == events_);
    lastEvents_ = events_;
    return res;
  }

  void update();
  void addToEpoll();
  void remove();

  void setReadHandler(Callback &&readHandler) { readHandler_ = readHandler; }
  void setWriteHandler(Callback &&writeHandler) {
    writeHandler_ = writeHandler;
  }
  void setErrorHandler(Callback &&errorHandler) {
    errorHandler_ = errorHandler;
  }
  void setConnHandler(Callback &&connHandler) { connHandler_ = connHandler; }
  void setCloseHandler(Callback &&closehandler) {
    closeHandler_ = closehandler;
  }

  // 根据发生的不同的数据类型执行不同的回调函数
  void handleEvents() {
    // std::cout << "Call Callback function" << std::endl;
    events_ = 0;
    // 如果发生的事件是对端关闭且没有写入数据
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
      events_ = 0;
      if (closeHandler_) closeHandler_();
      return;
    }
    if (revents_ & EPOLLERR) {
      if (errorHandler_) errorHandler_();
      events_ = 0;
      return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
      handleRead();
    }
    if (revents_ & EPOLLOUT) {
      handleWrite();
    }
    handleConn();
  }
  void handleRead();
  void handleWrite();
  void handleError(int fd, int err_num, std::string short_msg);
  void handleConn();
};