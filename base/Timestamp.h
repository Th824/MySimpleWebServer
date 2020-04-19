// 时间戳的封装，提供各种方法返回格式化的时间表示

# pragma once

# include "utility.h"
# include <cinttypes>
# include <algorithm>
# include <string>
# include <ctime>


// Timestamp中只存储一个微秒数，其他的格式从这个微秒数计算得来
class Timestamp {
public:
  static const int kMicroSecondsPerSecond = 1000 * 1000;

  Timestamp() : microSecondsSinceEpoch_(0) {}

  // 通常调用这个函数来构造时间戳，通过和now方法的配合
  // 示例用法：Timestamp time(addTime(Timestamp::now(), interval));
  explicit Timestamp(std::int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

  void swap(Timestamp& that) {
    std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
  }

  // 格式化函数
  std::string toString() const;
  std::string toFormattedString(bool showMicroseconds = true) const;

  bool valid() const {return microSecondsSinceEpoch_ > 0;}

  std::int64_t microSecondsSinceEpoch() const {return microSecondsSinceEpoch_;}
  std::time_t secondsSinceEpoch() const {return static_cast<std::time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);}

  static Timestamp now();
  static Timestamp invalid() {return Timestamp();}

  static Timestamp fromUnixTime(std::time_t t) {return fromUnixTime(t, 0);}
  static Timestamp fromUnixTime(std::time_t t, int microseconds) {
    return Timestamp(static_cast<std::int64_t>(t) * kMicroSecondsPerSecond);
  }
private:
  // 时间戳中只有一个表示微秒数的64位整型变量
  std::int64_t microSecondsSinceEpoch_;
};

// 为什么不用引用，为什么使用inline，为什么不是类的成员函数
inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double timeDifference(Timestamp high, Timestamp low) {
  std::int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
  std::int64_t delta = static_cast<std::int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
  return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}