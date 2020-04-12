# pragma once

# include <vector>
# include <map>
# include <sys/epoll.h>
# include <memory>
# include "Channel.h"
# include "base/noncopyable.h"

class Epoll : noncopyable {
public:
  Epoll();
  ~Epoll();
  // 往epoll中添加关注的事件
  void epoll_add(Channel* request, int timeout);
  // 修改epoll中的事件
  void epoll_mod(Channel* request, int timeout);
  // 删除epoll中的事件
  void epoll_del(Channel* request);

  std::vector<Channel*> poll();

private:
  static const int MAXFDS = 100000;
  int epollFd_;
  std::vector<epoll_event> events_;
  std::map<int, Channel*> fd2chan_;
  // TODO，定时器
  
};