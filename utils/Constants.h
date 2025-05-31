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
  static constexpr double InvalidPrice =
      std::numeric_limits<double>::quiet_NaN();
  static constexpr std::uint32_t PriceMultiplier = 100;

  // Runtime constant (initialized once at program startup)
  static const TimeStamp EndOfTime;
  // const TimeStamp EndOfTime;
};
