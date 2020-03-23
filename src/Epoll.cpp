# include <assert.h>
# include <iostream>
# include "Epoll.h"

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

Epoll::Epoll()
 :epollFd_(epoll_create1(EPOLL_CLOEXEC)),
  events_(EVENTSNUM) {
  assert(epollFd_ > 0);
}

Epoll::~Epoll() {
  // TODO，是否需要手动关闭文件描述符
}

void Epoll::epoll_add(SP_Channel request, int timeout) {
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getEvents();

  fd2chan_[fd] = request;
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
    fd2chan_[fd].reset();
  }
}

void Epoll::epoll_mod(SP_Channel request, int timeout) {
  int fd = request->getFd();
  // 减少对内核的操作
  if (!request->EqualAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      fd2chan_[fd].reset();
    }
  }
}

void Epoll::epoll_del(SP_Channel request) {
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getLastEvents();
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    // TODO 错误处理
  }
  fd2chan_[fd].reset();
}

// 返回活跃事件vector
std::vector<SP_Channel> Epoll::poll() {
  while (true) {
    // 将epoll返回的活跃事件写入events_
    int eventsNum = epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (eventsNum > 0) {
      std::cout << eventsNum << " connection arrive!" << std::endl;
    }
    if (eventsNum < 0) perror("epoll wait error");
    std::vector<SP_Channel> reqData;
    for (int i = 0; i < eventsNum; i++) {
      int fd = events_[i].data.fd;
      SP_Channel curReq = fd2chan_[fd];

      if (curReq) {
        std::cout << "The req is valid" << std::endl;
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