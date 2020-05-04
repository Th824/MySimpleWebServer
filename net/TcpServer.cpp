#include "net/TcpServer.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>

#include "base/utility.h"

#include <istream>

// const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;

void defaultConnCallback(const TcpConnectionPtr& conn) {
  LOG << conn->srcAddr() << ":" << conn->srcPort() << " -> "
            << conn->dstAddr() << ":" << conn->dstPort() << " is "
            << ((conn->connected()) ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf) {
  std::string message = buf->retrieveAllAsString();
  LOG << "read a message: " << message;
}

TcpServer::TcpServer(unsigned short port, EventLoop* loop)
    : srcPort_(port),
      // 根据端口号初始化acceptedFd
      listenFd_(socket_bind_listen(srcPort_)),
      loop_(loop),
      pool_(new EventLoopThreadPool(loop_)),
      acceptChannel_(new Channel(loop_, listenFd_)),
      started_(false) {
  setSocketNonBlocking(listenFd_);
}

void TcpServer::start() {
  pool_->start();
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
  acceptChannel_->setReadHandler(std::bind(&TcpServer::handleNewConn, this));
  // 将listenFd_加入到epoll中
  loop_->addToPoller(acceptChannel_, 0);
  started_ = true;
  loop_->loop();
}

void TcpServer::setConnCallback(const connCallback& cb = defaultConnCallback) {
  connCallback_ = cb;
}

void TcpServer::setMessageCallback(
    const messageCallback& cb = defaultMessageCallback) {
  messageCallback_ = cb;
}

void TcpServer::setwriteCompleteCallback(const writeCompleteCallback& cb) {
  writeCompleteCallback_ = cb;
}

void TcpServer::handleNewConn() {
  // 该函数是有新连接到来的回调函数
  // 当有新的socket连接到来的时候，调用该函数将新建的socket连接按照
  // Round-Robin算法分配给线程池中的线程
  struct sockaddr_in clientAddr;
  memset(&clientAddr, 0, sizeof(struct sockaddr_in));
  socklen_t addrLen = sizeof(clientAddr);
  int acceptFd = 0;

  // 每一次处理新连接都是处理到没有新连接为止(读到EAGAIN)
  while ((acceptFd = accept(listenFd_, (struct sockaddr*)(&clientAddr),
                            &addrLen)) > 0) {
    std::string dstAddr(inet_ntoa(clientAddr.sin_addr));
    unsigned short dstPort = ntohs(clientAddr.sin_port);
    // 超出最大连接数错误处理
    if (acceptFd >= MAXFDS) {
      close(acceptFd);
      continue;
    }

    // 将连接套接字设置为非阻塞模式
    if (setSocketNonBlocking(acceptFd) < 0) {
      LOG << "Set non block failed";
      return;
    }

    setSocketNodelay(acceptFd);
    // 从线程池中获取将该连接分配到的线程
    EventLoop* loop = pool_->getNextLoop();
    
    // 根据acceptFd建立Channel，将Channel加入唯一的Epoll中，直接操作Channel
    // 在创建了一个关于acceptChannel后，将其添加到选定的subReactor线程中
    // 根据acceptFd建立TcpConnection，并将其保存在TcpServer的connections中
    std::string connectionName =
        std::string("connection") + std::to_string(count_++);
    // LOG << "AcceptChannel " << acceptChannel_->getFd() << " accept a new connection " << acceptFd;
    TcpConnectionPtr connection(new TcpConnection(
        loop, connectionName, acceptFd, srcAddr_, dstAddr, srcPort_, dstPort));
    assert(connections_.find(connectionName) == connections_.end());
    connections_[connectionName] = connection;
    // 对新建的connection设置接收消息回调函数，在有消息到来时，会调用该函数
    connection->setMessageCallback(messageCallback_);
    // 对新建的connection设置连接
    connection->setConnCallback(connCallback_);
    // 设置写完成的回调函数
    connection->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置关闭连接的回调函数，该函数因为需要对TcpServer中的成员变量（connections）进行操作，定义在TcpServer中，同时
    // 也在其中调用了TcpConnection的connectDestroyed函数
    connection->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    loop->runInLoop(
        std::bind(&TcpConnection::connectionEstablished, connection));
  }
  // 因为设置了ONESHOT标志所以需要再次设置，需不需要再次设置ONESHOT呢？
  // acceptChannel_->setEvents(EPOLLIN| EPOLLET);
  // acceptChannel_->update();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
  loop_->assertInLoopThread();
  // LOG << "connection use_count is " << conn.use_count(); 
  connections_.erase(conn->name());
  // LOG << "Erase connection";
  // EventLoop* loop = conn->loop();
  // bind与lambda表达式的区别，将conn作为参数传入是否会改变share_ptr的引用计数
  // loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}