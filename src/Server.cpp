# include "Server.h"
# include "../base/utility.h"
# include <sys/socket.h>
# include <cstring>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <functional>
# include <iostream>

const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;

void readCalllback() {
  std::cout << "handle read" << std::endl;
}

void writeCallback() {
  std::cout << "handle write" << std::endl;
}

void connCallback() {
  std::cout << "handle conn" << std::endl;
}

Server::Server(int port, EventLoop *loop) : 
  port_(port),
  // 根据端口号初始化acceptedFd
  listenFd_(socket_bind_listen(port_)),
  loop_(loop),
  acceptChannel_(new Channel(loop_, listenFd_)),
  started_(false) {
    setSocketNonBlocking(listenFd_);
}

void Server::start() {
  acceptChannel_->setEvents(DEFAULT_EVENT);
  acceptChannel_->setReadHandler(std::bind(&Server::handleNewConn, this));
  acceptChannel_->setConnHandler(std::bind(&Server::handleThisConn, this));
  // 将listenFd_加入到epoll中
  loop_->addToPoller(acceptChannel_, 0);
  started_ = true;
  std::cout << "Start the server successfully" << std::endl;
}

void Server::handleThisConn() {
  loop_->updatePoller(acceptChannel_);
}

void Server::handleNewConn() {
  std::cout << "handle new connection!" << std::endl;
  // 该函数是有新连接到来的回调函数
  // 当有新的socket连接到来的时候，调用该函数将新建的socket连接按照
  // Round-Robin算法分配给线程池中的线程
  struct sockaddr_in clientAddr;
  memset(&clientAddr, 0, sizeof(struct sockaddr_in));
  socklen_t addrLen = sizeof(clientAddr);
  int acceptFd = 0;
  // 每一次处理新连接都是处理到没有新连接为止
  while ((acceptFd = accept(listenFd_, (struct sockaddr*)(&clientAddr), &addrLen)) > 0) {
    std::cout << "Accept a new connection successfully" << std::endl;
    // 从线程池中获取将该连接分配到的线程
    // EventLoop *loop = threadPool_->getNextLoop();

    // 将连接分配到唯一的loop
    
    // TODO
    // 日志处理，将连接记录到日志中
    
    // 超出最大连接数
    if (acceptFd >= MAXFDS) {
      close(acceptFd);
      continue;
    }

    // 将连接套接字设置为非阻塞模式
    if (setSocketNonBlocking(acceptFd) < 0) {
      // TODO
      // 日志处理
      return;
    }

    setSocketNodelay(acceptFd);
    // 根据acceptFd建立Channel，将Channel加入唯一的Epoll中，直接操作Channel
    SP_Channel acceptFdChannel(new Channel(loop_, acceptFd));
    acceptFdChannel->setEvents(DEFAULT_EVENT);
    acceptChannel_->setReadHandler(std::bind(&readCalllback));
    acceptChannel_->setWriteHandler(std::bind(&writeCallback));
    acceptChannel_->setConnHandler(std::bind(&connCallback));
    loop_->addToPoller(acceptFdChannel);
    // 只有一个EventLoop，因此目前不用实现插入到Loop中
    // loop_->queueInLoop()
    // TODO

  }
  acceptChannel_->setEvents(EPOLLIN| EPOLLET);
}