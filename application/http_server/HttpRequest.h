#pragma once
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/Buffer.h"

class HttpRequest {
 private:
  enum Method { GET, POST, PUT, DELETE, INVALID };
  enum Version { HTTP10, HTTP11, NONE };
  enum State { RequestLine, RequestHeader, RequestBody, Done };

 public:
  HttpRequest() : state_(RequestLine) {}
  ~HttpRequest() {}

  bool parseRequestLine(const char* begin, const char* end) {
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if (space != end) {
      if (!setMethod(start, space)) return succeed;
      start = space + 1;
      space = std::find(start, end, ' ');
      if (space != end) {
        const char* question = std::find(start, space, '?');
        if (question != space) {
          if (!setPath(start, question)) return succeed;
          if (!setQuery(question, space)) return succeed;
        } else {
          if (!setPath(start, space)) return succeed;
        }
        start = space + 1;
        succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
        if (succeed) {
          succeed = setVersion(start, end);
        }
      }
    }
    return succeed;
  }

  bool parseRequestHeader(const char* begin, const char* end) {
    bool succeed = false;
    const char* colon = std::find(begin, end, ':');
    if (colon != end) {
      std::string key(begin, colon);
      header_[key] = std::string(colon + 1, end);
      succeed = true;
    }
    return succeed;
  }

  bool isEmptyLine(const char* begin, const char* end) { return end == begin; }

  // 在onMessage中作为回调函数调用，在可读的时候被调用
  bool parseRequest(Buffer* buf) {
    bool isEnough = true;
    while (isEnough) {
      const char* crlf = buf->findCRLF();
      // 没有找到crlf，返回
      if (crlf == nullptr) break;
      // 找到了crlf，根据状态跳转
      if (state_ == RequestLine) {
        if (!parseRequestLine(buf->peek(), crlf)) return false;
        buf->retrieveUntil(crlf + 2);
        state_ = RequestHeader;
      } else if (state_ == RequestHeader) {
        // 找到了空行后跳出循环
        if (isEmptyLine(buf->peek(), crlf)) {
          buf->retrieveUntil(crlf + 2);
          state_ = RequestBody;
          break;
        } else {
          if (!parseRequestHeader(buf->peek(), crlf)) return false;
          buf->retrieveUntil(crlf + 2);
        }
      }
    }

    if (state_ == RequestBody) {
      if (header_.find("Content-Length") != header_.end()) {
        int contentLength = std::stoi(header_["Content-Length"]);
        assert(contentLength >= 0);
        if (buf->readableBytes() >= static_cast<size_t>(contentLength)) {
          // 一般对body的数据没有处理，直接retrieve
          buf->retrieve(contentLength);
        }
      }
      state_ = Done;
    }
    return true;
  }

  std::string state() const {
    if (state_ == RequestLine) return "RequestLine";
    if (state_ == RequestHeader) return "RequestHeader";
    if (state_ == RequestBody) return "RequestBody";
    if (state_ == Done) return "Done";
    return "Unknow";
  }

  std::string method() const {
    if (method_ == GET) return "GET";
    if (method_ == POST) return "POST";
    if (method_ == PUT) return "PUT";
    if (method_ == DELETE) return "DELETE";
    return "INVALID";
  }

  std::string version() const {
    if (version_ == HTTP10) return "HTTP/1.0";
    if (version_ == HTTP11) return "HTTP/1.1";
    return "NONE";
  }

  std::string path() const { return path_; }

  bool isKeepAlive() {
    if (header_.find("Connection") == header_.end() ||
        header_["Connection"] == "close" || header_["Connection"] == "Close" ||
        header_["connection"] == "close" || header_["connection"] == "Close")
      return false;
    return true;
  }

  bool setMethod(const char* begin, const char* end) {
    std::string method(begin, end);
    if (method == "GET") {
      method_ = GET;
    } else if (method == "POST") {
      method_ = POST;
    } else if (method == "PUT") {
      method_ = PUT;
    } else if (method == "DELETE") {
      method_ = DELETE;
    } else {
      method_ = INVALID;
    }
    return method_ != INVALID;
  }

  bool setVersion(const char* begin, const char* end) {
    std::string version(begin, end);
    if (version == "HTTP/1.0") {
      version_ = HTTP10;
    } else if (version == "HTTP/1.1") {
      version_ = HTTP11;
    } else {
      version_ = NONE;
    }
    return version_ != NONE;
  }

  bool setPath(const char* begin, const char* end) {
    path_ = std::string(begin, end);
    return true;
  }

  bool setQuery(const char* begin, const char* end) {
    query_ = std::string(begin, end);
    return true;
  }

  std::string getQuery() const { return query_; }

  void reset() {
    header_.clear();
    state_ = RequestLine;
  }

  friend std::ostream& operator<<(std::ostream& output,
                                  const HttpRequest& req) {
    output << req.method() << ' ' << "127.0.0.1:30000" << req.path();
    return output;
  }

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
};