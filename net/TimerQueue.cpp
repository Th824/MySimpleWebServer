# include <sys/timerfd.h>
# include <unistd.h>
# include <cstring>
# include <assert.h>

# include "TimerQueue.h"
# include "EventLoop.h"
# include "Timer.h"
# include "TimerID.h"

int createTimerfd() {
  int timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

  if (timerfd < 0) {

  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when) {
  std::int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100) {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

void readTimerfd(int timerfd, Timestamp now) {
  std::uint64_t howmany;
  ssize_t n = read(timerfd, &howmany, sizeof(howmany));
  // TODO 日志处理

}

void resetTimerfd(int timerfd, Timestamp expiration) {
  struct itimerspec newValue;
  struct itimerspec oldValue;
  std::memset(static_cast<void *>(&newValue), 0, sizeof(newValue));
  std::memset(static_cast<void *>(&oldValue), 0, sizeof(oldValue));
  // 设定初次过期的时间
  newValue.it_value = howMuchTimeFromNow(expiration);
  int ret = timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret) {
    // TODO 日志记录
  }
}

TimerQueue::TimerQueue(EventLoop *loop)
  : loop_(loop),
    timerfd_(createTimerfd()),
    timerfdChannel_(loop, timerfd_),
    timers_(),
    callingExpiredTimers_(false) {
  timerfdChannel_.setReadHandler(std::bind(&TimerQueue::handleRead, this));
  // timerfdChannel_.enableReading();
  timerfdChannel_.setDefaultEvents();
  timerfdChannel_.update();
}

TimerQueue::~TimerQueue() {
  timerfdChannel_.remove();
  close(timerfd_);
  for (const Entry& timer : timers_) {
    delete timer.second;
  }
}

TimerID TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval) {
  Timer *timer = new Timer(std::move(cb), when, interval);
  loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
  return TimerID(timer, timer->sequence());
}

void TimerQueue::cancel(TimerID timerID) {
  loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerID));
}

// 线程安全的调用方式
void TimerQueue::addTimerInLoop(Timer* timer) {
  loop_->assertInLoopThread();
  bool earliestChanged = insert(timer);

  // 如果插入的定时器是最快过期的定时器，更新timerfd
  if (earliestChanged) {
    resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::cancelInLoop(TimerID timerID) {
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  ActiveTimer timer(timerID.timer_, timerID.sequence_);
  ActiveTimerSet::iterator it = activeTimers_.find(timer);
  if (it != activeTimers_.end()) {
    // 如果定时器还没有到时
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1);
    (void)n;
    delete it->first;
    activeTimers_.erase(it);
  } else if (callingExpiredTimers_) {
    // 如果定时器已经到时且正在处理到时回调函数，那么将注销定时器加入到注销集合中
    cancelingTimers_.insert(timer);
  }
  assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
  Timestamp nextExpire;

  for (const Entry& it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
      it.second->restart(now);
      insert(it.second);
    } else {
      delete it.second;
    }
  }

  if (!timers_.empty()) {
    nextExpire = timers_.begin()->second->expiration();
  }

  if (nextExpire.valid()) {
    resetTimerfd(timerfd_, nextExpire);
  }
}

// timerfd在到期时会变得可读
void TimerQueue::handleRead() {
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  readTimerfd(timerfd_, now);

  std::vector<Entry> expired = getExpired(now);

  callingExpiredTimers_ = true;
  cancelingTimers_.clear();

  // 对到期的定时器调用其注册的到期回调函数
  for (const Entry& it : expired) {
    it.second->run();
  }
  callingExpiredTimers_ = false;
  reset(expired, now);
}

// 根据传入的当前时间来获取已注册的定时器中的过期定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
  assert(timers_.size() == activeTimers_.size());
  std::vector<Entry> expired;
  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  TimerList::iterator end = timers_.lower_bound(sentry);
  assert(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, std::back_inserter(expired));
  timers_.erase(timers_.begin(), end);
  
  for (const Entry& it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    // 返回值可能是0或者1
    size_t n = activeTimers_.erase(timer);
    assert(n == 1);
    // 使编译器不提示未使用变量n的warnning
    (void)n;
  }

  assert(timers_.size() == activeTimers_.size());
  return expired;
}

// 根据ID移除注册在queue中的timer
void TimerQueue::cancel(TimerID timerID) {
  loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerID));
}

bool TimerQueue::insert(Timer* timer) {
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  // 标志该timer是不是第一个过期的
  bool earliestChanged = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliestChanged = true;
  }
  {
    std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
    assert(result.second);
    (void)result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second);
    (void)result;
  }

  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}
