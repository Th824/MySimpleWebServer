# include <unistd.h>
# include <iostream>
# include <sys/socket.h>
# include <netinet/in.h>
#include <netinet/tcp.h>
# include <sys/types.h>
# include <fcntl.h>
# include "utility.h"

int socket_bind_listen(int port) {
  // 检查端口号是否合法
  if (port < 0 || port > __UINT16_MAX__) return -1;

  int listenFd = 0;
  // 获取socket描述符
  if ((listenFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    std::cout << "Create socket fail" << std::endl;
    return -1;
  }
  // 强制将socket绑定到port端口，解决无法绑定端口的问题
  int opt = 1;
  if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
    close(listenFd);
    return -1;
  }
  // 绑定socket描述符
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(listenFd, (struct sockaddr*)(&address), sizeof(address)) < 0) {
    std::cout << "Bind port fail" << std::endl;
    close(listenFd);
    return -1;
  }

  // 开始监听端口，支持的最大连接数是2048
  if (listen(listenFd, 2048) < 0) {
    std::cout << "Listen fail" << std::endl;
    close(listenFd);
    return -1;
  }
  std::cout << "The socket has run in " << "127.0.0.1:" << port << std::endl;

  // 上面操作都成功则返回该文件描述符供后续操作
  return listenFd;
}

int setSocketNonBlocking(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1) return -1;

  flag |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flag) == -1) return -1;
  return 0;
}

void setSocketNodelay(int fd) {
  int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}