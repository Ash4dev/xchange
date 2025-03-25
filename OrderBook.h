#pragma once

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>

#include "Level.h"
#include "Order.h"
#include "Trade.h"
#include "utils/alias/Fundamental.h"
#include "utils/alias/LevelRel.h"
#include "utils/alias/OrderRel.h"
#include "utils/enums/Side.h"

class OrderBook {
private:
  std::map<Price, LevelPointer, std::greater<Price>> m_bids;
  std::map<Price, LevelPointer, std::less<Price>> m_asks;

public:
  OrderBook() = default;

  std::optional<Trade> AddOrder(Order &order);
  std::optional<Trade> CancelOrder(Order &order);
  std::optional<Trade> ModifyOrder(Order &oldOrder, Order &modifiedOrder);

  bool CanMatchOrder(Side::Side side, Price price) const;
  std::optional<Trade> MatchPotentialOrders();

  std::map<Price, LevelPointer, std::greater<Price>> getBidLevels() const {
    return m_bids;
  };
  std::map<Price, LevelPointer, std::less<Price>> getAskLevels() const {
    return m_asks;
  };
};
