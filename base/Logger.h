#pragma once

#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "../base/noncopyable.h"
#include "AsyncLogger.h"
#include "LogStream.h"

class Logger {
 public:
  Logger(const std::string& fileName, int line) : impl_(fileName, line) {}
  ~Logger();
  LogStream& stream() { return impl_.stream_; }

  // 用来控制是否输出到控制台
  static bool outputToConsole;
  static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
  static std::string getLogFileName() { return logFileName_; }
  static void once_init();
  static void output(const std::string&);

 private:
  class Impl {
   public:
    Impl(const std::string& fileName, int line);
    void formatTime();

    LogStream stream_;
    int line_;
    std::string basename_;
  };
  Impl impl_;
  static std::string logFileName_;
  // 所有的Logger前端公用一个后端，所以设置成静态成员变量
  static AsyncLogger AsyncLogger_;
  static std::once_flag once_control;
};

#define LOG Logger(__FILE__, __LINE__).stream()
// #define LOG std::cout
