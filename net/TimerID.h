// 对Timer和sequence的封装

# include "base/copyable.h"
# include <cinttypes>

class Timer;

class TimerID : public copyable {
public:
  TimerID()
    : timer_(nullptr),
      sequence_(0) {}

  TimerID(Timer *timer, std::int64_t seq)
    : timer_(timer),
      sequence_(seq) {}

  friend class TimerQueue;

private:
  Timer* timer_;
  std::int64_t sequence_;
};