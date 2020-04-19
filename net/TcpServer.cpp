# include "net/TcpServer.h"
# include "base/utility.h"
# include <sys/socket.h>
# include <cstring>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <functional>
# include <iostream>

// const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;

void defaultConnCallback(const TcpConnectionPtr& conn) {
  std::cout << conn->srcAddr() << ":" << conn->srcPort() << " -> "
            << conn->dstAddr() << ":" << conn->dstPort() << " is "
            << ((conn->connected()) ? "UP" : "DOWN") << std::endl;
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf) {
  std::string message = buf->retrieveAllAsString();
  std::cout << "read a message: " << message << std::endl;
}

void httpMessageCallback(const TcpConnectionPtr& conn, Buffer* buf) {
  std::shared_ptr<HttpRequest> httpRequest = conn->httpRequest();
  httpRequest->parseRequest(buf);
  if (httpRequest->state() == "Done") {
    HttpRespond httpRespond;
    httpRespond.setStateCode("200");
    httpRespond.setBody("hello world");
    httpRespond.setContentLength(11);
    conn->send(httpRespond.generateRespond());
    httpRequest->reset(); 
  }
}

TcpServer::TcpServer(unsigned short port, EventLoop *loop) : 
  srcPort_(port),
  // 根据端口号初始化acceptedFd
  listenFd_(socket_bind_listen(srcPort_)),
  loop_(loop),
  acceptChannel_(new Channel(loop_, listenFd_)),
  started_(false),
  pool_(new EventLoopThreadPool(loop_)) {
    setSocketNonBlocking(listenFd_);
}

void TcpServer::start() {
  pool_->start();
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
  acceptChannel_->setReadHandler(std::bind(&TcpServer::handleNewConn, this));
  // 将listenFd_加入到epoll中
  loop_->addToPoller(acceptChannel_, 0);
  started_ = true;
  std::cout << "Start the TcpServer successfully" << std::endl;
}

void TcpServer::setConnCallback(const connCallback& cb = defaultConnCallback) {
  connCallback = cb;
}

void TcpServer::setMessageCallback(const messageCallback& cb = defaultMessageCallback) {
  messageCallback = cb;
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
  while ((acceptFd = accept(listenFd_, (struct sockaddr*)(&clientAddr), &addrLen)) > 0) {
    // // 日志处理，将连接记录到日志中
    std::cout << "New connection from " << inet_ntoa(clientAddr.sin_addr) << ":"
              << ntohs(clientAddr.sin_port) << std::endl;
    std::string dstAddr(inet_ntoa(clientAddr.sin_addr));
    unsigned short dstPort = ntohs(clientAddr.sin_port);
    // 超出最大连接数
    if (acceptFd >= MAXFDS) {
      close(acceptFd);
      continue;
    }

    // 将连接套接字设置为非阻塞模式
    if (setSocketNonBlocking(acceptFd) < 0) {
      std::cout << "Set non block failed" << std::endl;
      return;
    }

    setSocketNodelay(acceptFd);
    // 从线程池中获取将该连接分配到的线程
    EventLoop *loop = pool_->getNextLoop();
    // 根据acceptFd建立Channel，将Channel加入唯一的Epoll中，直接操作Channel
    // 在创建了一个关于acceptChannel后，将其添加到选定的subReactor线程中
    // 根据acceptFd建立TcpConnection，并将其保存在TcpServer的connections中
    std::string connectionName = std::string("connection") + std::to_string(count_++);
    TcpConnectionPtr connection(new TcpConnection(loop, connectionName, acceptFd, srcAddr_, dstAddr, srcPort_, dstPort));
    assert(connections_.find(connectionName) == connections_.end());
    connections_[connectionName] = connection;
    connection->setMessageCallback(httpMessageCallback);
    connection->setConnCallback(defaultConnCallback);
    connection->setCloseCallback(std::bind(&TcpServer::removeConnection, this, connection));
    loop->runInLoop(std::bind(&TcpConnection::connectionEstablished, connection));
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
  connections_.erase(conn->name());
  EventLoop* loop = conn->loop();
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}