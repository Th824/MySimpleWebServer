# pragma once
# include <vector>
# include <string>
# include <unordered_map>
# include "Buffer.h"

class HttpRequest {
private:
  enum Method {GET, POST, PUT, DELETE, INVALID};
  enum Version {HTTP10, HTTP11, NONE};
  enum State {RequestLine, RequestHeader, RequestBody, Done};
public:
  HttpRequest() : state_(RequestLine) {}
  ~HttpRequest() {}

  bool parseRequestLine(const char* begin, const char* end) {
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end)
    {
      if (!setMethod(start, space)) return succeed;
      start = space + 1;
      space = std::find(start, end, ' ');
      if (space != end)
      {
        if (!setPath(start, space)) return succeed;
        start = space + 1;
        succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
        if (succeed)
        {
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

  bool isEmptyLine(const char* begin, const char* end) {
    return end == begin;
  }

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
        if (buf->readableBytes() >= contentLength) {
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
  }

  std::string method() const {
    if (method_ == GET) return "GET";
    if (method_ == POST) return "POST";
    if (method_ == PUT) return "PUT";
    if (method_ == DELETE) return "DELETE";
    return "INVALID";
  }

  std::string version() const {
    if (version_ == HTTP10) return "HTTP1.0";
    if (version_ == HTTP11) return "HTTP1.1";
    return "NONE";
  }

  std::string path() const {
    return path_;
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

  void reset() {
    header_.clear();
    state_ = RequestLine;
  }

private:
  State state_;
  // HTTP请求行的字段
  Method method_;
  Version version_;
  std::string path_;
  // HTTP
  std::unordered_map<std::string, std::string> header_;
};