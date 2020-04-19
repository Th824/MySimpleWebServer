# pragma once
# include "base/noncopyable.h"
# include "base/Callback.h"
# include "EventLoop.h"
# include "Channel.h"
# include "EventLoopThreadPool.h"
# include "Buffer.h"
// # include "HttpRequest.h" // todo using context to replace this function
# include "application/http_server/HttpRequest.h"
# include <memory>
# include <cassert>
# include <sys/uio.h>
# include <string>
# include <unistd.h>
# include <functional>
# include <algorithm>
# include <sys/socket.h>

// 定义一次TCP连接，顶层应用只需要传入回调函数即可
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {
private:
  enum State{kDisconnected, kConnecting, kConnected, kDisconnecting};

public:
  TcpConnection(EventLoop* loop, const std::string& name, int fd, std::string srcAddr, std::string dstAddr, unsigned short srcPort, unsigned short dstPort);
  ~TcpConnection() {}

  void connectionEstablished();
  void connectDestroyed();

  bool connected() const {return state_ == kConnected;}

  void shutdown() {
    // FIXME: use compare and swap
    if (state_ == kConnected) {
      setState(kDisconnecting);
      loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
  }

  void shutdownInLoop() {
    loop_->assertInLoopThread();
    // if (!channel_->isWriting()) {
    //   // we are not writing
    //   socket_->shutdownWrite();
    // }
  }

  void setState(State state) {state_ = state;}

  // send的操作一般都是在回调函数中被调用的
  void send(const void* message, int len) {
    if (state_ == kConnected) {
      if (loop_->isInLoopThread()) {
        sendInLoop(message, len);
      }
    }
  }

  void send(const char* message, int len) {
    send(static_cast<const void*>(message), len);
  }

  void send(const std::string& message) {
    send(message.c_str(), message.length());
  }
  // void send(Buffer *message);

  void sendInLoop(const void* data, size_t len) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    // bool faultError = false;
    // !channel_->isWriting() &&
    if (outputBuffer_.readableBytes() == 0) {
      // 如果outputBuffer中没有需要写出去的（保证写入socket内核缓冲区的顺序），则可以直接调用write
      nwrote = write(channel_->getFd(), data, len);
      if (nwrote >= 0) {
        remaining = len - nwrote;
        // 如果全部写完且有定义写完的回调函数，则调用回调函数
        if (remaining == 0 && writeCompleteCallback_) {
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
      } else {
        nwrote = 0;
        // 错误处理
      }
    }

    assert(remaining <= len);
    // 将没有成功发送出去的添加到outputBuffer中
    if (remaining > 0) {
      size_t oldLen = outputBuffer_.readableBytes();
      outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
      // if (!channel_->isWriting()) {
      //   channel_->enableWriting();
      // }
    }
  }

  const std::string getState() const {
    switch(state_) {
      case kDisconnected:
        return "kDisconnected";
      case kConnected:
        return "kConnected";
      case kConnecting:
        return "kConnecting";
      case kDisconnecting:
        return "kDisconnecting";
      default:
        return "unknown state";
    }
  }

  // 这三个回调函数应该由应用程序传入定义，不同应用层协议（不同应用）应该传入不同的回调函数
  void setConnCallback(const connCallback& cb) {connCallback_ = cb;}
  void setMessageCallback(const messageCallback& cb) {messageCallback_ = cb;}
  void setCloseCallback(const closeCallback& cb) {closeCallback_ = cb;}
  void setWriteCompleteCallback(const writeCompleteCallback& cb) {writeCompleteCallback_ = cb;}

  Buffer* inputBuffer() {return &inputBuffer_;}
  Buffer* outputBuffer() {return &outputBuffer_;}

  std::string srcAddr() const {return srcAddr_;}
  std::string dstAddr() const {return dstAddr_;}
  unsigned short srcPort() const {return srcPort_;}
  unsigned short dstPort() const {return dstPort_;}
  std::string name() const {return name_;}
  EventLoop* loop() const {return loop_;}
  std::shared_ptr<HttpRequest> httpRequest() const {return httpRequest_;}
private:
  std::shared_ptr<HttpRequest> httpRequest_;
  // 表示当前状态
  State state_;
  // using readCallback = std::function<void (const TcpConnectionPtr&)>;

  // 由四元组来定义一个TCP连接
  std::string srcAddr_, dstAddr_;
  unsigned short srcPort_, dstPort_;
  std::string name_;
  int fd_;
  // 定义两个缓冲区，应用程序直接往缓冲区中写数据即可，由缓冲区来负责把数据写入内核
  // 此缓冲区不是tcp socket内核缓冲区
  Buffer inputBuffer_, outputBuffer_;
  std::unique_ptr<Channel> channel_;
  // 该连接所属的EventLoop/IO线程
  EventLoop *loop_;
  // 回调函数
  connCallback connCallback_;
  writeCompleteCallback writeCompleteCallback_;
  messageCallback messageCallback_;
  closeCallback closeCallback_;

  // 以下四个函数都是channel的回调函数，通过设置这四个函数，在不同的事件发生
  // 的时候调用不同的回调函数
  // 从socket中读取数据
  void handleRead() {
    loop_->assertInLoopThread();
    int savedErrno = 0;
    // inputBuffer_从socket中读取，读到EAGAIN为止
    ssize_t n = inputBuffer_.readFd(channel_->getFd(), &savedErrno);
    if (n > 0) {
      // 如果取到数据，调用向上应用层的回调
      messageCallback_(shared_from_this(), &inputBuffer_);
    } else if (n == 0) {
      // 在对端发送了FIN后，read的返回值是0，此时调用关闭回调函数
      handleClose();
    } else {
      errno = savedErrno;
      std::cout << "TcpConnection::handleRead";
      handleError();
    }
  }
  // 将数据写到socket中，最可能发生阻塞的地方
  void handleWrite() {
    loop_->assertInLoopThread();
    ssize_t n = write(channel_->getFd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        if (writeCompleteCallback_) {
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        }
        // 这里主要是解决调用关闭连接后缓冲区仍然有数据待发送的问题
        if (state_ == kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      std::cout << "TcpConnection::handleWrite" << std::endl;
    }
  }
  void handleClose() {
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    // 清空channel的所有监听事件
    TcpConnectionPtr guardThis(shared_from_this());
    connCallback_(guardThis);
    // 当channel关闭时，需要调用TcpConnection的关闭回调函数
    closeCallback_(guardThis);
  }
  void handleError() {
    std::cout << "TcpConnection::handleError" << std::endl;
  }
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;