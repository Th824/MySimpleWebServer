#include "LogFile.h"
#include <iostream>

LogFile::LogFile(const std::string& basename, int flushEveryN)
    : basename_(basename),
      flushEveryN_(flushEveryN),
      count_(0),
      mutex_(new std::mutex) {
  file_.reset(new AppendFile(basename));
}

LogFile::~LogFile() {}

void LogFile::append(const std::string& logline) {
  std::lock_guard<std::mutex> lck_(*mutex_);
  file_->append(logline);
  ++count_;
  if (count_ >= flushEveryN_) {
    count_ = 0;
    file_->flush();
  }
}

void LogFile::flush() {
  std::lock_guard<std::mutex> lck_(*mutex_);
  file_->flush();
}
