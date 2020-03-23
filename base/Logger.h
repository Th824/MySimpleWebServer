# pragma once

# include "../base/noncopyable.h"
# include "AsyncLogger.h"
# include "LogStream.h"
# include <vector>
# include <string>
# include <cstring>
# include <memory>
# include <mutex>
# include <thread>
# include <condition_variable>
# include <functional>
# include <assert.h>
# include <chrono>
# include <fstream>

class Logger {
public:
  Logger(const std::string& fileName, int line) : impl_(fileName, line) {}
  ~Logger();
  LogStream &stream() {return impl_.stream_;}

  static void setLogFileName(std::string fileName) {logFileName_ = fileName;}
  static std::string getLogFileName() {return logFileName_;}
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
  static AsyncLogger AsyncLogger_;
  static std::once_flag once_control;
};

#define LOG Logger(__FILE__, __LINE__).stream()