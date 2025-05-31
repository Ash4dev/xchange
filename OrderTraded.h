#pragma once

#include "utils/alias/Fundamental.h"

struct OrderTraded {
  OrderID orderID;
  double price;
  Quantity quantityFilled;
};
