# include "FileUnit.h"

void AppendFile::append(const std::string& logline) {
  fp_ << logline;
}

void AppendFile::flush() {
  fp_ << std::flush;
}