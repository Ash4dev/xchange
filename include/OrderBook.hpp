#pragma once

#include <functional>
#include <map>
#include <optional>

#include "include/Order.hpp"
#include "include/Trade.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/LevelRel.hpp"
#include "utils/enums/Side.hpp"

class OrderBook {
private:
  Symbol m_symbol;
  std::map<Price, LevelPointer, std::greater<Price>> m_bids; // max bid tradable
  std::map<Price, LevelPointer, std::less<Price>> m_asks;    // min ask tradable

  // all trades that occur (reset mechanism & backup needed for atleast this)
  std::vector<Trade> m_trades;

public:
  // various constructors
  OrderBook() = default;
  OrderBook(Symbol symbol) : m_symbol{symbol} {}
  // linker error initially since copy constr not implemented before
  OrderBook(OrderBook &ob) = default; // copy constructor

  // made static since stay same across all instances
  static Price decodePriceFromOrderID(const OrderID orderID);
  static Side::Side decodeSideFromOrderID(const OrderID orderID);

  // core functionality of orderbook
  std::optional<Trade> AddOrder(Order &order);
  std::optional<Trade> CancelOrder(OrderID orderID);
  std::optional<Trade> ModifyOrder(OrderID orderID, Order &modifiedOrder);

  void printOrderBookState(const std::string &message = "");

  LevelPointer getLevelFromSideAndPrice(Side::Side side, Price price) const {
    // m_bids[price] not allowed since ob is constant
    // if [], can lead to creation of new level in that map
    if (side == Side::Side::Buy)
      return m_bids.at(price);
    else
      return m_asks.at(price);
  }

  // matching functionality
  bool CanMatchOrder(Side::Side side, Price price) const;
  std::optional<Trade> MatchPotentialOrders();

  // store the levels for each side
  std::map<Price, LevelPointer, std::greater<Price>> getBidLevels() const {
    return m_bids;
  };
  std::map<Price, LevelPointer, std::less<Price>> getAskLevels() const {
    return m_asks;
  };

  Symbol getSymbol() const { return m_symbol; } // symbol getter
  std::vector<Trade> getTrades() { return m_trades; }
};
