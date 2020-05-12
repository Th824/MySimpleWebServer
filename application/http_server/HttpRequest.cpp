#include "HttpRequest.h"

bool HttpRequest::parseRequestLine(const char* begin, const char* end) {
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

// 传入的是Content-type键值对的值部分
bool HttpRequest::parseContentType(const char* begin, const char* end) {
  const char* semicolon;
  while (*begin == ' ') begin++;
  while ((semicolon = std::find(begin, end, ';')) != end) {
    const char* equalSign = std::find(begin, semicolon, '=');
    if (equalSign == semicolon) {
      type_ = std::string(begin, semicolon - 1);
    } else {
      std::string key(begin, equalSign - 1);
      std::string value(equalSign + 1, semicolon - 1);
      if (key == "charset") {
        charset_ = value;
      } else if (key == "boundary") {
        boundary_ = value;
      } else {
        LOG << "Unexpected Content type";
        return false;
      }
    }
    begin = semicolon + 1;
    while (*begin == ' ') begin++;
  }
  return true;
}

bool HttpRequest::parseRange(const std::string& s) {
  // 该正则表达式用来获取range中的第一个range区间
  static auto reFirstRange = std::regex(R"(bytes=(\d*-\d*(?:,\s*\d*-\d*)*))");
  std::smatch m;
  if (std::regex_match(s, m, reFirstRange)) {
    auto pos = static_cast<size_t>(m.position(1));
    auto len = static_cast<size_t>(m.length(1));
    bool all_valid_ranges = true;
    split(&s[pos], &s[pos + len], ',', [&](const char* begin, const char* end) {
      if (all_valid_ranges) return;
      static auto re_another_range = std::regex(R"(\s*(\d*)-(\d*))");
      std::cmatch cm;
      if (std::regex_match(begin, end, cm, re_another_range)) {
        ssize_t first = -1;
        if (!cm.str(1).empty()) {
          first = static_cast<ssize_t>(std::stoll(cm.str(1)));
        }

        ssize_t last = -1;
        if (!cm.str(2).empty()) {
          last = static_cast<ssize_t>(std::stoll(cm.str(2)));
        }

        if (first != -1 && last != -1 && first > last) {
          all_valid_ranges = false;
          return;
        }
        ranges_.emplace_back(first, last);
      }
    });
    return all_valid_ranges;
  }
  return false;
}

bool HttpRequest::parseRequestHeader(const char* begin, const char* end) {
  bool succeed = false;
  const char* colon = std::find(begin, end, ':');
  if (colon != end) {
    std::string key(begin, colon);
    std::string value(colon + 1, end);
    if (key == "Content-type") {
      parseContentType(colon + 1, end);
    } else if (key == "Range") {
      parseRange(value);
    }
    header_[key] = value;
    succeed = true;
  }
  return succeed;
}

bool HttpRequest::isEmptyLine(const char* begin, const char* end) {
  return end == begin;
}

bool HttpRequest::parseRequest(Buffer* buf) {
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

  // 解析请求体，有可能是multipart类型
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

const std::string HttpRequest::state() const {
  if (state_ == RequestLine) return "RequestLine";
  if (state_ == RequestHeader) return "RequestHeader";
  if (state_ == RequestBody) return "RequestBody";
  if (state_ == Done) return "Done";
  return "Unknow";
}

const std::string HttpRequest::method() const {
  if (method_ == GET) return "GET";
  if (method_ == POST) return "POST";
  if (method_ == PUT) return "PUT";
  if (method_ == DELETE) return "DELETE";
  return "INVALID";
}

const std::string HttpRequest::path() const { return path_; }

const std::string HttpRequest::version() const {
  if (version_ == HTTP10) return "HTTP/1.0";
  if (version_ == HTTP11) return "HTTP/1.1";
  return "NONE";
}

const std::string HttpRequest::body() const { return body_; }

bool HttpRequest::isKeepAlive() {
  if (header_.find("Connection") == header_.end() ||
      header_["Connection"] == "close" || header_["Connection"] == "Close" ||
      header_["connection"] == "close" || header_["connection"] == "Close")
    return false;
  return true;
}

bool HttpRequest::setMethod(const char* begin, const char* end) {
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

bool HttpRequest::setVersion(const char* begin, const char* end) {
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

bool HttpRequest::setPath(const char* begin, const char* end) {
  path_ = std::string(begin, end);
  return true;
}

bool HttpRequest::setQuery(const char* begin, const char* end) {
  query_ = std::string(begin, end);
  return true;
}

const std::string HttpRequest::getQuery() const { return query_; }

void HttpRequest::reset() {
  header_.clear();
  state_ = RequestLine;
}