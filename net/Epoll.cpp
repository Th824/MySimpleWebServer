#include "Epoll.h"
#include "base/Logger.h"
#include <assert.h>
#include <iostream>

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
  assert(epollFd_ > 0);
}

Epoll::~Epoll() {
  // TODO，是否需要手动关闭文件描述符
}

void Epoll::epoll_add(Channel* request, int timeout) {
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getEvents();
  // 添加成功
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == 0) {
    fd2chan_[fd] = request;
  }
}

void Epoll::epoll_mod(Channel* request, int timeout) {
  int fd = request->getFd();
  // 减少对内核的操作
  if (!request->EqualAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      fd2chan_.erase(fd);
    }
  }
}

void Epoll::epoll_del(Channel* request) {
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getLastEvents();
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    // TODO 错误处理
  }
  fd2chan_.erase(fd);
}

// 返回活跃事件vector
std::vector<Channel*> Epoll::poll() {
  while (true) {
    // 将epoll返回的活跃事件写入events_
    // 最后一个参数表示超时时间，0表示立即返回（会陷入busy
    // loop），-1表示只有当有事件产生才返回，正数t表示在有事件产生或者t秒后返回
    int eventsNum =
        epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (eventsNum < 0) perror("epoll wait error");
    std::vector<Channel*> reqData;
    for (int i = 0; i < eventsNum; i++) {
      int fd = events_[i].data.fd;
      Channel* curReq = fd2chan_[fd];

      if (curReq) {
        curReq->setRevents(events_[i].events);
        curReq->setEvents(0);
        reqData.push_back(curReq);
      } else {
        // TODO，日志记录
      }
    }
    if (reqData.size() > 0) return reqData;
  }
}