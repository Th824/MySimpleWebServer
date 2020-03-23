#pragma once
#include "noncopyable.h"
#include "LogStream.h"
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <assert.h>
#include <chrono>
#include <fstream>

class AsyncLogger : noncopyable {
public:
  AsyncLogger(const std::string& filename)
   : running_(false),
     thread_(nullptr),
     filename_(filename),
     currentBuffer_(new Buffer),
     nextBuffer_(new Buffer) {
       buffers_.reserve(16);
  };
  ~AsyncLogger() {
    if (running_)
      stop();
  }

  void append(const std::string &logline);

  void start() {
    running_ = true;
    thread_ = new std::thread(std::bind(&AsyncLogger::threadFunc, this));
    std::unique_lock<std::mutex> lck(countdownMutex_);
    countdownLatch_.wait(lck);
    // thread_->join();
  }

  void stop() {
    running_ = false;
    cond_.notify_one();
    thread_->join();
  }

private:
  void threadFunc();

  typedef FixedBuffer<kLargeSize> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  bool running_;
  std::thread* thread_;
  // 用来互斥下面四个变量的使用
  std::mutex mutex_;
  std::mutex countdownMutex_;
  std::condition_variable cond_;
  std::condition_variable countdownLatch_;
  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;
  std::string filename_;
};