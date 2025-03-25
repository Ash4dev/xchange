#pragma once

#include "utils/alias/Fundamental.h"

struct OrderTraded {
  OrderID orderID;
  Price price;
  Quantity quantityFilled;
};
