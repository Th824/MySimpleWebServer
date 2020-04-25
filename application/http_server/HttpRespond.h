#pragma once
#include <sstream>
#include <string>
#include <unordered_map>

class HttpRespond {
 public:
  HttpRespond() {}
  ~HttpRespond() {}

  void setStateCode(const std::string& stateCode) { stateCode_ = stateCode; }

  void setStatusMessage(const std::string& message) {
    statusMessage_ = message;
  }

  void setBody(const std::string& body) { body_ = body; }

  void setHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
  }

  std::string generateRespond() {
    std::stringstream ss;
    ss << version_ << ' ' << stateCode_ << ' ' << statusMessage_ << crlf;
    if (closeConnection_) {
      ss << "Connection: close" << crlf;
    } else {
      ss << "Connection: Keep-Alive" << crlf;
    }

    for (const auto& kv : headers) {
      ss << kv.first << ": " << kv.second << crlf;
    }
    ss << crlf;
    ss << body_;
    return ss.str();
  }

  std::string& getBody() { return body_; }

  friend std::ostream& operator<<(std::ostream& output,
                                  const HttpRespond& res) {
    output << res.stateCode_ << ' ' << res.statusMessage_;
    return output;
  }

 private:
  std::string crlf = "\r\n";
  std::string version_ = "HTTP/1.1";
  std::string stateCode_;
  std::string statusMessage_ = "OK";
  std::unordered_map<std::string, std::string> headers;
  bool closeConnection_ = false;
  std::string body_;
};