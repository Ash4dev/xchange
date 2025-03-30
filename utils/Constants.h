#pragma once

#include "alias/Fundamental.h"
#include <limits>

struct Constants {
  // orders w/ Market order use this
  // orders are stored in a level (have a price)
  // InvalidPrice of type Price fits the bill
  static const Price InvalidPrice = std::numeric_limits<Price>::quiet_NaN();
};
