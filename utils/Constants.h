#pragma once

#include "alias/Fundamental.h"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <limits>

struct Constants {
  // Orders with Market order are equivalent to GoodTillCancel(worst Price)

  // Compile-time constants
  // static constexpr double InvalidPrice =
  //     std::numeric_limits<double>::quiet_NaN();
  static constexpr Price InvalidPrice = 1e9;

  // static constexpr vs inline
  inline static const TimeStamp EndOfTime = [] {
    std::tm tm{};
    tm.tm_year = 2100 - 1900; // Year since 1900
    tm.tm_mon = 0;            // January (0-based)
    tm.tm_mday = 1;           // 1st day of the month
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1; // Let the system determine DST

    // Convert std::tm to std::time_t
    std::time_t tt = std::mktime(&tm);

    // Convert to std::chrono::system_clock::time_point
    return std::chrono::system_clock::from_time_t(tt);
  }();
  ;
  // Runtime constant (initialized once at program startup)
  // discard Constants.cpp since more of obstruction than help
};
