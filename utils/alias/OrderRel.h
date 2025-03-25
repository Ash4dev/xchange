#pragma once

#include "../../Order.h"
#include <list>
#include <memory>

using OrderPointer = std::shared_ptr<Order>;
using OrderList = std::list<OrderPointer>;
