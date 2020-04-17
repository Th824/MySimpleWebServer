# pragma once
# include <string>
# include <sstream>

class HttpRespond {
public:
  HttpRespond() {}
  ~HttpRespond() {}

  void setStateCode(const std::string& stateCode) {
    stateCode_ = stateCode;
  }

  void setContentLength(const int contentLength) {
    contentLength_ = contentLength;
  }

  void setBody(const std::string& body) {
    body_ = body;
  }

  std::string generateRespond() {
    std::stringstream ss;
    ss << version_ << ' ' << stateCode_ << "OK" << crlf
       << "TimeStamp" << crlf
       << "Content-Type: text/html;charset=utf-8" << crlf
       << "Content-Length: " << contentLength_ << crlf
       << crlf
       << body_;
    return ss.str();
  }

private:
  std::string crlf = "\r\n";
  std::string version_ = "HTTP/1.1";
  std::string stateCode_;
  int contentLength_;
  std::string body_;
};