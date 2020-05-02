#include "HttpServer.h"

void HttpServer::start() {
  setMessageCallback();
  setConnCallback();
  setWriteCompleteCallback();
  tcpServer_.start();
}

// Get请求可能是请求文件，也可能是请求数据库中的某些信息
HttpServer& HttpServer::Get(const std::string& pattern,
                            const HttpHandler& httpHandler) {
  // 使用pattern创建regex对象，并将其加入getHandlers_中
  getHandlers_.emplace_back(std::regex(pattern), httpHandler);
  return *this;
}

// 设置URL相关请求对应的锚点
// 例如针对file文件夹中example文件的请求file/example，我们可以将file前缀锚定到服务器的static文件夹，而后file请求都会锚定到static文件夹
bool HttpServer::setMountPoint(const std::string& mp, const std::string& dir) {
  // 将路径拼成绝对路径
  std::string fullPath = staticFilePathPrefix_ + dir;
  // 判断dir文件夹是否存在
  if (isDir(fullPath)) {
    if (!mp.empty() && mp[0] == '/') {
      mountToDir_.emplace_back(mp, dir);
      return true;
    }
  }
  return false;
}

void HttpServer::setMessageCallback() {
  tcpServer_.setMessageCallback(
      [this](const TcpConnectionPtr& conn, Buffer* buf) {
        // 获取当前TCP连接中正在处理的请求指针
        std::shared_ptr<HttpRequest> httpRequest = conn->httpRequest();
        // 解析请求，可能还没有获取到完整的头部信息
        httpRequest->parseRequest(buf);
        // 判断解析请求头部是否完成
        if (httpRequest->state() == "Done") {
          // 在获取到了完整的请求后，调用相关的路由函数，路由函数中会修改传入的respond
          HttpRespond httpRespond;
          // 根据已注册的路由方法进行路由
          this->routingHandler(*httpRequest, httpRespond);
          LOG << httpRequest->method() << ' ' << this->host_ << ':'
              << this->port_ << httpRequest->path() << ' '
              << httpRespond.statusCode() << ' ' << httpRespond.statusMessage();
          conn->setKeepAlive(httpRequest->isKeepAlive());
          // 异步发送
          // EventLoop* loop = conn->loop();
          // loop->queueInLoop(
          //     std::bind(&TcpConnection::send, conn,
          //               httpRespond.generateRespond(), std::placeholders::_1));
          // std::string strRespond = httpRespond.generateRespond();
          // loop->runInLoop(std::bind(&TcpConnection::send, conn, strRespond, strRespond.length()));
          conn->send(httpRespond.generateRespond());
          httpRequest->reset();
        }
      });
}

void HttpServer::setConnCallback() {
  tcpServer_.setConnCallback([this](const TcpConnectionPtr& conn) {
    LOG << conn->srcAddr() << ":" << conn->srcPort() << " -> "
        << conn->dstAddr() << ":" << conn->dstPort() << " is "
        << ((conn->connected()) ? "UP" : "DOWN");
  });
}

void HttpServer::setWriteCompleteCallback() {
  tcpServer_.setwriteCompleteCallback([this](const TcpConnectionPtr& conn) {
    EventLoop* loop = conn->loop();
    // 如果不是持久连接，在写完成后主动shutdown连接
    if (!conn->isKeepAlive()) {
      loop->queueInLoop(std::bind(&TcpConnection::shutdown, conn));
    }
  });
}

bool HttpServer::handleForFile(const HttpRequest& req, HttpRespond& res) {
  std::string path = req.path();
  for (auto const& kv : mountToDir_) {
    const std::string& mount = kv.first;
    const std::string& dir = kv.second;
    // 前缀匹配，如果找到，返回0
    if (!path.find(mount)) {
      std::string sub_path = path.substr(mount.size());
      if (isValidPath(sub_path)) {
        auto fullPath = staticFilePathPrefix_ + dir + "/" + sub_path;
        if (fullPath.back() == '/') {
          fullPath += "index.html";
        }

        if (isFile(fullPath)) {
          // 如果文件存在，则读取文件，并装入res的body中
          readFile(fullPath, res.getBody());
          auto type = findContentType(fullPath);
          if (!type.empty()) {
            res.setHeader("Content-Type", type);
            res.setHeader("Content-Length",
                          std::to_string(res.getBody().length()));
          }
          res.setVersion(req.version());
          res.setstatusCode("200");
          res.setStatusMessage("OK");
          return true;
        } else {
          LOG << "Not exist such file";
        }
      } else {
        LOG << "Invalid path";
      }
    } else {
      LOG << "Not mount such dir";
    }
  }
  res.setstatusCode("404");
  res.setStatusMessage("Not Found");
  return false;
}

// TODO 暂时默认不会出错
void HttpServer::readFile(const std::string& path, std::string& body) {
  std::ifstream fs(path, std::ios_base::binary);
  fs.seekg(0, std::ios_base::end);
  auto size = fs.tellg();
  fs.seekg(0);
  body.resize((static_cast<size_t>(size)));
  fs.read(&body[0], size);
}

bool HttpServer::routingHandler(const HttpRequest& req, HttpRespond& res) {
  std::string path = req.path();
  std::string method = req.method();
  // 处理文件请求
  if (method == "GET") {
    return handleForFile(req, res);
  }

  // 处理Get请求，有可能是请求文件
  if (method == "GET") {
    return dispatchRequest(req, res, getHandlers_);
  }
  return false;
}

bool HttpServer::dispatchRequest(
    const HttpRequest& req, HttpRespond& res,
    std::vector<std::pair<std::regex, HttpHandler>>& handlers) {
  try {
    for (const auto& x : handlers) {
      const auto& pattern = x.first;
      const auto& handler = x.second;

      if (std::regex_match(
              req.path(),
              /*req.matches,*/ pattern)) {  // todo: 在计算线程进行运算
        // handler中可能会涉及到计算复杂度高的操作，或者需要IO的操作
        handler(req, res);
        return true;
      }
    }
  } catch (const std::exception& ex) {
    // 处理出错
    res.setstatusCode("500");
    res.setHeader("EXCEPTION_WHAT", ex.what());
  } catch (...) {
    res.setstatusCode("500");
    res.setHeader("EXCEPTION_WHAT", "UNKNOWN");
  }
  return false;
}
