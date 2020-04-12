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