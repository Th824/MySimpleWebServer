# pragma once
# include <vector>
# include <mutex>
# include <set>
# include "base/Timestamp.h"
# include "base/Callback.h"
# include "base/noncopyable.h"
# include "Channel.h"

class EventLoop;
class Timer;
class TimerID;

class TimerQueue : noncopyable {
public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  TimerID addTimer(TimerCallback cb, Timestamp when, double interval);

  void cancel(TimerID timerID);
private:
  // 表示时间戳和定时器的二元组
  using Entry = std::pair<Timestamp, Timer*>;
  // 用集合来表示二元组的集合
  using TimerList = std::set<Entry>;
  // 表示的是Timer及其对应的存活时间？
  using ActiveTimer = std::pair<Timer*, std::int64_t>;
  // 依然存活的Timer的集合
  using ActiveTimerSet = std::set<ActiveTimer>;

  // 
  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerID timerID);
  // 可读回调函数，其实就是Timer过期处理函数
  void handleRead();
  // 获取过期的Entry
  std::vector<Entry> getExpired(Timestamp now);
  // 对过期的Entry进行重置
  void reset(const std::vector<Entry>& expired, Timestamp now);
  // 插入传入的timer
  bool insert(Timer* timer);

  EventLoop* loop_;
  // 只有一个timerfd
  const int timerfd_;
  Channel timerfdChannel_;
  // 表示当前存活的定时器集合，集合的元素是pair<Timestamp, Timer*>
  // 表示在该集合中，元素是按照Timestamp进行排序的，便于定位到超时的定时器
  TimerList timers_;

  // 表示当前存活的定时器集合，且集合的元素是pair<Timer*, std::int64_t>
  // 表示在该集合中，元素是按照timer的地址排序的
  ActiveTimerSet activeTimers_;
  std::atomic_bool callingExpiredTimers_; // atomic
  // 表示注销的定时器集合
  ActiveTimerSet cancelingTimers_;
};