// 该头文件主要用来定义各种回调函数的签名

# include <functional>
# include <memory>

class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using connCallback = std::function<void(const TcpConnectionPtr &)>;
using writeCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
// 应用层下发给传输层的信息就是一条信息
using messageCallback = std::function<void(const TcpConnectionPtr &, Buffer *)>;
using closeCallback = std::function<void(const TcpConnectionPtr &)>;

using TimerCallback = std::function<void()>;