#include "OrderBook.h"
#include "Level.h"
#include "Trade.h"
#include "utils/alias/Fundamental.h"
#include "utils/alias/OrderRel.h"
#include "utils/enums/Side.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <ostream>

void OrderBook::AddOrder(Order &order) {
  Price price = order.getPrice();
  Side::Side side = order.getSide();

  if (side == Side::Side::Buy) {
    if (m_bids.find(price) == m_bids.end()) {
      Level newBidLevel = Level(price, 0);
      m_bids[price] = newBidLevel;
    }
    m_bids[price].AddOrder(order);
  } else {
    if (m_asks.find(price) == m_asks.end()) {
      Level newAskLevel = Level(price, 0);
      m_asks[price] = newAskLevel;
    }
    m_asks[price].AddOrder(order);
  }
}

void OrderBook::CancelOrder(Order &order) {
  Price price = order.getPrice();
  Side::Side side = order.getSide();
  OrderID orderID = order.getOrderID();

  if (side == Side::Side::Buy) {
    m_bids[price].CancelOrder(orderID);
    if (m_bids[price].getQuantity() == 0)
      m_bids.erase(price);
  } else {
    m_asks[price].CancelOrder(orderID);
    if (m_asks[price].getQuantity() == 0)
      m_asks.erase(price);
  }
}

void OrderBook::ModifyOrder(Order &oldOrder, Order &modifiedOrder) {
  OrderBook::CancelOrder(oldOrder);
  OrderBook::AddOrder(modifiedOrder);
}

bool OrderBook::CanMatchOrder(Side::Side side, Price price) const {
  // ternary operator fails for incompatible types
  // m_bids and m_asks are incompatible types: comparators
  if (side == Side::Side::Buy) {
    if (m_asks.empty())
      return false;
    Level bestAskLevel = (*m_asks.begin()).second;
    return (price >= bestAskLevel.getPrice());
  } else {
    if (m_bids.empty())
      return false;
    Level bestBidLevel = (*m_bids.begin()).second;
    return (price <= bestBidLevel.getPrice());
  }
}

// https://www.learncpp.com/cpp-tutorial/stdoptional/
std::optional<Trade> OrderBook::MatchPotentialOrders() {

  if ((m_bids.empty() || m_asks.empty())) {
    return std::nullopt;
  }

  Level bestBidLevel = (*m_bids.begin()).second;
  Level bestAskLevel = (*m_asks.begin()).second;

  if (bestBidLevel.getPrice() < bestAskLevel.getPrice()) {
    return std::nullopt;
  }

  OrderPointer bestBidOrder = bestBidLevel.getOrderList().front();
  OrderPointer bestAskOrder = bestAskLevel.getOrderList().front();

  Quantity filledQuantity = std::min(bestBidOrder->getRemainingQuantity(),
                                     bestAskOrder->getRemainingQuantity());

  Price settlementPrice = bestAskLevel.getPrice();

  bestBidOrder->FillPartially(filledQuantity);
  bestAskOrder->FillPartially(filledQuantity);
  bestBidLevel.UpdateLevelQuantityPostMatch(filledQuantity);
  bestAskLevel.UpdateLevelQuantityPostMatch(filledQuantity);
  std::cout << "bid: " << bestBidLevel.getPrice() << " "
            << bestBidLevel.getQuantity() << " ask: " << bestAskLevel.getPrice()
            << " " << bestAskLevel.getQuantity() << std::endl;

  if (bestBidOrder->isFullyFilled()) {
    OrderID bidOrderID = bestBidOrder->getOrderID();
    bestBidLevel.removeMatchedOrder(bidOrderID);
    std::cout << "Yo BID!" << std::endl;
    if (m_bids[bestBidLevel.getPrice()].getOrderList().empty()) {
      std::cout << "Delete BidLevel!" << std::endl;
      m_bids.erase(bestBidLevel.getPrice());
    }
  }
  if (bestAskOrder->isFullyFilled()) {
    OrderID askOrderID = bestAskOrder->getOrderID();
    bestAskLevel.removeMatchedOrder(askOrderID);
    std::cout << "Yo ASK!" << std::endl;
    std::cout << m_asks[bestAskLevel.getPrice()].getQuantity() << std::endl;
    if (m_asks[bestAskLevel.getPrice()].getOrderList().empty()) {
      std::cout << "Delete AskLevel!" << std::endl;
      m_asks.erase(bestAskLevel.getPrice());
    }
  }

  OrderTraded bidTrade{bestBidOrder->getOrderID(), settlementPrice,
                       filledQuantity};
  OrderTraded askTrade{bestAskOrder->getOrderID(), settlementPrice,
                       filledQuantity};
  return Trade{bidTrade, askTrade};
}
