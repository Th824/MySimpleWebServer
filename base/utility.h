# pragma once
# include <sys/types.h>
# include <string>
# include <iostream>

// 根据端口号建立socket，并返回属于该socket的文件描述符
int socket_bind_listen(int port);

// 当一个进程向某个已收到RST的套接字执行写操作时，内核向该进程发送一个SIGPIPE信号。
// 该信号的默认行为是终止进程，因此进程必须捕获它以免不情愿地被终止。
void handle_sigpipe();

// 将套接字设置为非阻塞，设置为非阻塞是因为我们使用epoll来监听套接字的连接，当接收到epoll的
// 的通知时，说明确实有连接事件到来，因此直接使用非阻塞的连接套接字即可。
int setSocketNonBlocking(int fd);

void setSocketNodelay(int fd);

// IO接口
ssize_t readn(int fd, void *buff, size_t n);
ssize_t readn(int fd, std::string &inBuffer, bool &zero);
ssize_t readn(int fd, std::string &inBuffer);
ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string &sbuff);

bool isDir(const std::string& path);
bool isFile(const std::string& path);
bool isValidPath(const std::string &path);

std::string fileExtension(const std::string &path);
std::string findContentType(const std::string& path);

template <class Fn>
void split(const char* begin, const char* end, char delemiter, Fn fn);