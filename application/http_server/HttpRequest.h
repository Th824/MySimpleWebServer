#pragma once
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>

#include "net/Buffer.h"

class HttpRequest {
 private:
  enum Method { GET, POST, PUT, PATCH, DELETE, INVALID };
  enum Version { HTTP10, HTTP11, NONE };
  enum State { RequestLine, RequestHeader, RequestBody, Done };

 public:
  HttpRequest() : state_(RequestLine) {}
  ~HttpRequest() {}

  bool parseRequestLine(const char* begin, const char* end);
  bool parseRequestHeader(const char* begin, const char* end);
  bool isEmptyLine(const char* begin, const char* end);
  // 在onMessage中作为回调函数调用，在可读的时候被调用
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

  // friend std::ostream& operator<<(std::ostream& output,
  //                                 const HttpRequest& req) {
  //   output << req.method() << ' ' << "127.0.0.1:30000" << req.path();
  //   return output;
  // }

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
  // 判断类型是否是multipart
  bool isMultiPart;
};