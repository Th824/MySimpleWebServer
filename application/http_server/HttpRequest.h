#pragma once
#include <algorithm>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/Buffer.h"

class HttpRequest {
 private:
  enum Method { GET, POST, PUT, PATCH, DELETE, INVALID };
  enum Version { HTTP10, HTTP11, NONE };
  enum State { RequestLine, RequestHeader, RequestBody, Done };

  using Range = std::pair<ssize_t, ssize_t>;
  using Ranges = std::vector<Range>;

 public:
  HttpRequest() : state_(RequestLine) {}
  ~HttpRequest() {}

  // 解析请求行
  bool parseRequestLine(const char* begin, const char* end);
  // 解析请求头
  bool parseRequestHeader(const char* begin, const char* end);
  bool isEmptyLine(const char* begin, const char* end);
  // 在onMessage中作为回调函数调用，在可读的时候被调用
  bool parseRange(const std::string& s);
  bool parseContentType(const char* begin, const char* end);
  // 解析请求
  bool parseRequest(Buffer* buf);

  const std::string state() const;
  const std::string method() const;
  const std::string version() const;
  const std::string path() const;
  const std::string getQuery() const;
  const std::string body() const;
  bool isKeepAlive();

  bool setMethod(const char* begin, const char* end);
  bool setVersion(const char* begin, const char* end);
  bool setPath(const char* begin, const char* end);
  bool setQuery(const char* begin, const char* end);
  bool setBody(const char* begin, const char* end);

  void reset();

 private:
  State state_;
  // HTTP请求行的字段
  Method method_;
  Version version_;
  std::smatch match_;
  std::string path_;
  std::string query_;
  // HTTP
  std::unordered_map<std::string, std::string> header_;
  std::string body_;
  // std::multimap<std::string, std::string> header_;
  // 对于multipart类型的解析
  bool isMultiPart_;
  // Content-type字段
  std::string type_;
  std::string boundary_;
  std::string charset_;
  // Range字段
  Ranges ranges_;
};