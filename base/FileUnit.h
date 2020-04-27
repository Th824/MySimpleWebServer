#pragma once
#include <fstream>
#include <iostream>
#include <string>
#include "Logger.h"
#include "noncopyable.h"

// 对底层的文件进行封装
class AppendFile : noncopyable {
 public:
  explicit AppendFile(std::string filename) {
    fp_.open(filename, std::ios_base::app | std::ios_base::out);
    fp_.rdbuf()->pubsetbuf(buffer_, 64 * 1024);
    if (!fp_) {
      LOG << "open file fail";
    }
  }
  ~AppendFile() { fp_.close(); }

  void append(const std::string& logline);

  void flush();

 private:
  std::ofstream fp_;
  char buffer_[64 * 1024];
};