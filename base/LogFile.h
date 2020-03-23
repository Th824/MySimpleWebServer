#pragma once
# include "noncopyable.h"
# include "FileUnit.h"
# include <memory>
# include <string>
# include <mutex>

class LogFile : noncopyable {
public:
  LogFile(const std::string& basename, int flushEveryN = 1024);
  ~LogFile();

  void append(const std::string& logline);
  void flush();
  // bool rollFile();
private:
  void append_unlocked(const std::string& logline);

  const std::string basename_;
  const int flushEveryN_;

  int count_;
  std::unique_ptr<AppendFile> file_;
  std::unique_ptr<std::mutex> mutex_;
};