#pragma once
#include <sstream>
#include <string>
#include <unordered_map>

class HttpRespond {
 public:
  HttpRespond() {}
  ~HttpRespond() {}

  void setstatusCode(const unsigned short& statusCode);
  void setBody(const std::string& body);
  void setHeader(const std::string& key, const std::string& value);
  void setVersion(const std::string& version);
  void setRequestFilePath(const std::string& requestFilePath) {
    requestFilePath_ = requestFilePath;
  }

  std::string generateRespond();

  const std::string& getRequestFilePath() const { return requestFilePath_; }
  std::string& getBody() { return body_; }
  unsigned short statusCode() const { return statusCode_; }
  std::string statusMessage() const { return statusMessage_; }

  static const char* statusToMessage(unsigned short status);

  // friend std::ostream& operator<<(std::ostream& output,
  //                                 const HttpRespond& res) {
  //   output << res.statusCode_ << ' ' << res.statusMessage_;
  //   return output;
  // }

 private:
  std::string crlf = "\r\n";
  std::string version_ = "HTTP/1.1";
  unsigned short statusCode_ = 200;
  std::string statusMessage_ = "OK";
  std::unordered_map<std::string, std::string> headers;
  bool closeConnection_ = false;
  std::string requestFilePath_;
  std::string body_;
};