#pragma once
#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "noncopyable.h"

const int kSmallSize = 4000;
const int kLargeSize = 4000 * 1000;

template <int SIZE>
class FixedBuffer : noncopyable {
 public:
  FixedBuffer() : data_(SIZE, '\0'), cur_index(0){};
  ~FixedBuffer(){};

  // TODO
  void append(const std::string& buf) {
    if (avail() > buf.length()) {
      data_.replace(data_.begin() + cur_index,
                    data_.begin() + cur_index + buf.length(), buf);
      cur_index += buf.length();
    }
  };

  size_t length() const { return cur_index; };
  size_t avail() const { return SIZE - cur_index - 1; };
  const std::string data() const { return data_.substr(0, cur_index); };
  void reset() { cur_index = 0; };

 private:
  std::string data_;
  int cur_index;
};

class LogStream : noncopyable {
 public:
  typedef FixedBuffer<kSmallSize> Buffer;

  LogStream& operator<<(short v) {
    *this << std::to_string(static_cast<int>(v));
    return *this;
  }
  LogStream& operator<<(unsigned short v) {
    *this << std::to_string(static_cast<unsigned int>(v));
    return *this;
  }
  LogStream& operator<<(int v) {
    *this << std::to_string(v);
    return *this;
  }
  LogStream& operator<<(unsigned int v) {
    *this << std::to_string(v);
    return *this;
  }
  LogStream& operator<<(long v) {
    *this << std::to_string(v);
    return *this;
  }
  LogStream& operator<<(unsigned long v) {
    *this << std::to_string(v);
    return *this;
  }
  LogStream& operator<<(long long v) {
    *this << std::to_string(v);
    return *this;
  }
  LogStream& operator<<(unsigned long long v) {
    *this << std::to_string(v);
    return *this;
  }

  LogStream& operator<<(const void*);

  LogStream& operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
  }

  LogStream& operator<<(double v) {
    *this << std::to_string(v);
    return *this;
  }
  LogStream& operator<<(long double v) {
    *this << std::to_string(v);
    return *this;
  }

  LogStream& operator<<(char v) {
    buffer_.append(std::string(1, v));
    return *this;
  }

  LogStream& operator<<(const char* str) {
    if (str) {
      // TODO may happen bug, why?
      buffer_.append(std::string(str));
    } else {
      buffer_.append("(null)");
    }
    return *this;
  }

  LogStream& operator<<(const unsigned char* str) {
    return operator<<(reinterpret_cast<const char*>(str));
  }

  LogStream& operator<<(const std::string& v) {
    buffer_.append(v);
    return *this;
  }

  // 向外提供接口
  void append(const std::string& data) { buffer_.append(data); }
  const Buffer& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

 private:
  // 用来将多个输入连接成一条的缓冲区
  Buffer buffer_;
};
