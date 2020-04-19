// Timer类，主要用来表示一个定时器，内含一个时间戳，以及一个回调函数
// 

# pragma once
# include "base/Timestamp.h"
# include "base/noncopyable.h"
# include "base/Callback.h"
# include <atomic>


class Timer : noncopyable {
public:
  Timer(TimerCallback cb, Timestamp when, double interval)
    : callback_(std::move(cb)),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0),
      sequence_(s_numCreated_++) {}
    
    void run() const {callback_();}
    Timestamp expiration() const {return expiration_;}
    bool repeat() const {return repeat_;}
    std::int64_t sequence() const {return sequence_;}

    void restart(Timestamp now);

    static std::int64_t numCreated() {return s_numCreated_;}
private:
  const TimerCallback callback_;
  Timestamp expiration_;  // 定义过期的时间
  const double interval_;
  const bool repeat_;
  const std::int64_t sequence_;

  // 用来表示Timer ID的静态变量，因为在多线程环境中，需要使用原子变量或者加锁互斥
  static std::atomic_int64_t s_numCreated_;
};