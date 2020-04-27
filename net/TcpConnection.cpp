#include "TcpConnection.h"

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int fd,
                             std::string srcAddr, std::string dstAddr,
                             unsigned short srcPort, unsigned short dstPort)
    : state_(kConnecting),
      srcAddr_(srcAddr),
      dstAddr_(dstAddr),
      srcPort_(srcPort),
      dstPort_(dstPort),
      name_(name),
      fd_(fd),
      channel_(new Channel(loop, fd)),
      loop_(loop),
      httpRequest_(new HttpRequest) {
  // 对下层对应的channel设置回调事件
  channel_->setReadHandler(std::bind(&TcpConnection::handleRead, this));
  channel_->setWriteHandler(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseHandler(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorHandler(std::bind(&TcpConnection::handleError, this));
  // 设置socket为keepalive
  int optval = 1;
  setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
             static_cast<socklen_t>(sizeof(optval)));
}

void TcpConnection::connectionEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  // 连接建立改变状态，且设置channel监听的事件，并将channel添加到EPOLL中
  setState(kConnected);
  channel_->setDefaultEvents();
  // 使用queueInLoop避免了epoll在wait中阻塞，且不会造成冲突
  loop_->queueInLoop([this]() { this->channel_->addToEpoll(); });
  connCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();

  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->setNoneEvents();
    connCallback_(shared_from_this());
  }
  closeCallback_.~closeCallback();
  channel_->remove();
  // fd的生命周期由TcpConnection管理
  close(fd_);
}
