#pragma once

#include "utils/alias/Fundamental.hpp"

struct OrderTraded {
  OrderID orderID;
  double price;
  Quantity quantityFilled;
};
