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
  channel_->setErrorHandler(std::bind(&TcpConnection::handleError, this));
  channel_->setCloseHandler(std::bind(&TcpConnection::handleClose, this));
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

void TcpConnection::  connectDestroyed() {
  loop_->assertInLoopThread();
  int n = 0;
  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->setNoneEvents();
    connCallback_(shared_from_this());
  }
  channel_->remove();
  // fd的生命周期由TcpConnection管理
  n = close(fd_);
  if (n < 0) {
    LOG << "Close error: " << errno;
  }
  LOG << "Close socket " << fd_ << " and send FIN";
}

// 连接的主动关闭，需要自顶向下关闭，包括TcpConnection，Channel，socketFd，Epoll中事件的清除
void TcpConnection::shutdown() {
  // FIXME: use compare and swap
  if (state_ == kConnected) {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  LOG << "Shut down socket " << fd_;
  if (::shutdown(fd_, SHUT_WR) < 0) {
    LOG << "Shut down Tcp Connection error";
  }
  // sleep(5);
}

// send的操作一般都是在回调函数中被调用的
void TcpConnection::send(const void* message, int len) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message, len);
    }
  }
}

void TcpConnection::send(const char* message, int len) {
  send(static_cast<const void*>(message), len);
}

void TcpConnection::send(const std::string& message) {
  send(message.c_str(), message.length());
}

void TcpConnection::sendInLoop(const void* data, size_t len) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  if (outputBuffer_.readableBytes() == 0) {
    // 如果outputBuffer中没有需要写出去的（保证写入socket内核缓冲区的顺序），则可以直接调用write
    nwrote = write(channel_->getFd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      // 如果全部写完且有定义写完的回调函数，则调用回调函数
      if (remaining == 0 && writeCompleteCallback_) {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      // 错误处理
    }
  }

  assert(remaining <= len);
  if (remaining == 0) {
    LOG << "Send to peer successfully";
  }
  // 将没有成功发送出去的添加到outputBuffer中
  if (remaining > 0) {
    // size_t oldLen = outputBuffer_.readableBytes();
    outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
  }
}
