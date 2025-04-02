#include "Constants.h"
#include <chrono>

// ChatGPT generated (needed help badly)
// sys_days not working in chrono for some reason

const TimeStamp Constants::EndOfTime = [] {
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
