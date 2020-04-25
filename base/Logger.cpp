#include "Logger.h"

#include <sys/time.h>

#include "AsyncLogger.h"

// static AsyncLogger *AsyncLogger_;
std::once_flag Logger::once_control;
std::string Logger::logFileName_ = "./Log.log";
AsyncLogger Logger::AsyncLogger_("./Log.log");

void Logger::once_init() {
  std::cout << "init anync logger" << std::endl;
  // AsyncLogger_ = new AsyncLogger(Logger::getLogFileName());
  Logger::AsyncLogger_.start();
}

void Logger::output(const std::string& msg) {
  std::call_once(once_control, once_init);
  AsyncLogger_.append(msg);
}

Logger::Impl::Impl(const std::string& fileName, int line)
    : stream_(), line_(line), basename_(fileName) {
  formatTime();
}

void Logger::Impl::formatTime() {
  struct timeval tv;
  time_t time;
  char str_t[26] = {0};
  gettimeofday(&tv, NULL);
  time = tv.tv_sec;
  struct tm* p_time = localtime(&time);
  strftime(str_t, 26, "%Y-%m-%d %H:%M:%S ", p_time);
  stream_ << str_t;
}

Logger::~Logger() {
  // 进行格式化
  impl_.stream_ << " -- " << impl_.basename_ << ":" << impl_.line_ << '\n';
  const LogStream::Buffer& buf(stream().buffer());
  output(buf.data());
}