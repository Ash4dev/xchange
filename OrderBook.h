#pragma once

#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <set>
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
  Symbol m_symbol;
  std::map<Price, LevelPointer, std::greater<Price>> m_bids;
  std::map<Price, LevelPointer, std::less<Price>> m_asks;
  struct AdditionDueComparator {
    const OrderBook *orderBook;
    explicit AdditionDueComparator(const OrderBook *ob) : orderBook(ob) {}
    bool operator()(const OrderID &orderID1, const OrderID &orderID2);
  };

  struct CancellationDueComparator {
    const OrderBook *orderBook;
    explicit CancellationDueComparator(const OrderBook *ob) : orderBook(ob) {}
    bool operator()(const OrderID &orderID1, const OrderID &orderID2);
  };

  std::set<OrderID, OrderBook::AdditionDueComparator> addQueue;
  std::set<OrderID, OrderBook::CancellationDueComparator> cancelQueue;

public:
  OrderBook() = default;
  OrderBook(Symbol symbol) : m_symbol{symbol} {}

  // made static since stays same across all instance
  static Price decodePriceFromOrderID(OrderID orderID);
  static Side::Side decodeSideFromOrderID(OrderID orderID);

  std::optional<Trade> AddOrder(Order &order);
  std::optional<Trade> CancelOrder(OrderID orderID);
  std::optional<Trade> ModifyOrder(OrderID orderID, Order &modifiedOrder);

  LevelPointer getLevelFromSideAndPrice(Side::Side side, Price price) const {
    // m_bids[price] not allowed since ob is constant
    // if [], can lead to creation of new level in that map
    if (side == Side::Side::Buy)
      return m_bids.at(price);
    else
      return m_asks.at(price);
  }

  bool CanMatchOrder(Side::Side side, Price price) const;
  std::optional<Trade> MatchPotentialOrders();

  std::map<Price, LevelPointer, std::greater<Price>> getBidLevels() const {
    return m_bids;
  };
  std::map<Price, LevelPointer, std::less<Price>> getAskLevels() const {
    return m_asks;
  };

  Symbol getSymbol() const { return m_symbol; }
};
