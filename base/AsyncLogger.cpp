# include "AsyncLogger.h"
# include "LogFile.h"

void AsyncLogger::append(const std::string &logline) {
  std::unique_lock<std::mutex> guard(mutex_);
  if (currentBuffer_->avail() > logline.length()) {
    currentBuffer_->append(logline);
  } else {
    buffers_.push_back(std::move(currentBuffer_));
    if (nextBuffer_) {
      currentBuffer_ = std::move(nextBuffer_);
    } else {
      // 很少发生这种情况
      currentBuffer_.reset(new Buffer);
    }
    currentBuffer_->append(logline);
    cond_.notify_one();
  }
}

void AsyncLogger::threadFunc() {
  std::cout << "run thread function" << std::endl;
  assert(running_ == true);
  std::cout << "run thread function1" << std::endl;
  LogFile output(filename_);
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  std::cout << "run thread function2" << std::endl;
  bool flag = true;
  while (running_) {
    if (flag) {
      flag = false;
      std::unique_lock<std::mutex> lck(countdownMutex_);
      countdownLatch_.notify_one();
    }
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      // 使用c++11中的unique_lock
      std::unique_lock<std::mutex> guard(mutex_);
      auto flushInterval = std::chrono::system_clock::now() + std::chrono::seconds(3);
      cond_.wait_until(guard, flushInterval, [this]()->bool {
        return !this->buffers_.empty();
      });
      buffers_.push_back(std::move(currentBuffer_));
      currentBuffer_ = std::move(newBuffer1);
      buffersToWrite.swap(buffers_);
      if (!nextBuffer_) {
        nextBuffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    // 如果有大量的待写buffer
    if (buffersToWrite.size() > 25) {

    }

    for (const auto& buffer : buffersToWrite) {
      output.append(buffer->data());
    }

    if (buffersToWrite.size() > 2) {
      buffersToWrite.resize(2);
    }

    if (!newBuffer1) {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2) {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}