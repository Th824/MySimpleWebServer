# pragma once
# include "base/noncopyable.h"
# include "base/Callback.h"
# include "EventLoop.h"
# include "Channel.h"
# include "EventLoopThreadPool.h"
# include <memory>
# include <cassert>
# include <sys/uio.h>
# include <string>
# include <unistd.h>
# include <functional>
# include <algorithm>

// 应用层缓冲区
// 应用只需要将数据写到Buffer，并且将数据从Buffer读出，而避免了
// 从fd读写数据而造成阻塞使得无法快速回到loop
class Buffer {
public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;
  // 涉及到explict关键字的使用，只能显式调用该函数
  explicit Buffer(size_t initialSize = kInitialSize)
    : buffer_(kCheapPrepend + initialSize),
      readIndex_(kCheapPrepend),
      writeIndex_(kCheapPrepend) {
    
  }

  size_t readableBytes() const {return writeIndex_ - readIndex_;}
  size_t writableBytes() const {return buffer_.size() - writeIndex_;}
  size_t prependableBytes() const {return readIndex_;}
  const char* peek() const {return begin() + readIndex_;}
  const char* findCRLF() const {
    const char* ret = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return ret == beginWrite()? nullptr: ret;
  }

  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      readIndex_ += len;
    } else {
      retrieveAll();
    }
  }

  void retrieveUntil(const char* end) {
    assert(peek() < end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  void retrieveAll() {
    readIndex_ = writeIndex_ = kCheapPrepend;
  }

  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }

  std::string retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  void append(const char* data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  void append(const void* data, size_t len) {
    append(static_cast<const char*>(data), len);
  }

  // 在写入数据前需确保vector足够大来存储待写入的数据
  // 如果vector空间不够则申请空间
  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }
  char* beginWrite() {return begin() + writeIndex_;}
  const char* beginWrite() const {return begin() + writeIndex_;}
  void hasWritten(size_t len) {
    assert(len <= writableBytes());
    writeIndex_ += len;
  }
  void unwrite(size_t len) {
    assert(len <= readableBytes());
    writeIndex_ -= len;
  }

  void prepend(const void* data, size_t len) {
    assert(len <= prependableBytes());
    readIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + readIndex_);
  }

  ssize_t readFd(int fd, int* savedErrno) {
    // char extrabuf[65536];
    // struct iovec vec[2];
    // const size_t writable = writableBytes();
    // vec[0].iov_base = begin() + writeIndex_;
    // vec[0].iov_len = writable;
    // vec[1].iov_base = extrabuf;
    // vec[1].iov_len = sizeof(extrabuf);

    // const int iovcnt = (writable < sizeof(extrabuf))? 2 : 1;
    // const ssize_t n = readv(fd, vec, iovcnt);
    // if (n < 0) {
    //   *savedErrno = errno;
    // } else if (n <= writable) {
    //   writeIndex_ += n;
    // } else {
    //   writeIndex_ = buffer_.size();
    //   append(extrabuf, n - writable);
    // }
    // 使用edge trigger方式触发读取socket中的数据，因此，需要完全读取完socket缓冲区中的数据
    // 使用while循环来读取，每次读取的缓冲区大小为4096 bytes（栈上空间）
    /*
      On success, the number of bytes read is returned (zero indicates end
      of file), and the file position is advanced by this number.
    */
    ssize_t n = 0, sum = 0;
    char buf[4096];
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
      sum += n;
      append(buf, n);
    }
    // 读取到了文件结尾，即对面发出关闭socket信号
    if (n == 0) {
      // assert(sum == 0);
      return 0;
    }
    // 读取发生错误，返回-1，且判断errno，正常情况应该读取到发生EAGAIN error
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      assert(sum > 0);
      return sum;
    }
    *savedErrno = errno;
    return n;
  }
private:
  char kCRLF[3] = "\r\n";
  char* begin() {return &*buffer_.begin();}
  const char* begin() const {return &*buffer_.begin();}

  void makeSpace(size_t len) {
    // 如果空间不足，可用空间包括前面的prepend和写指针后面的大小之和
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      buffer_.resize(writeIndex_ + len);
    } else {
      // 空间足够则将数据往前移动到kCheapPrepend
      assert(kCheapPrepend < readIndex_);
      size_t readable = readableBytes();
      std::copy(begin() + readIndex_, begin() + writeIndex_, begin() + kCheapPrepend);
      readIndex_ = kCheapPrepend;
      writeIndex_ = readIndex_ + readable;
      assert(readable == readableBytes());
    }
  }
  std::vector<char> buffer_;
  size_t readIndex_;
  size_t writeIndex_;
};