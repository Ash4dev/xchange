#include "OrderBook.h"
#include "Level.h"
#include "Trade.h"
#include "utils/alias/Fundamental.h"
#include "utils/alias/LevelRel.h"
#include "utils/alias/OrderRel.h"
#include "utils/enums/Side.h"

#include <cassert>
#include <memory>
#include <optional>

void OrderBook::AddOrder(Order &order) {
  Price price = order.getPrice();
  Side::Side side = order.getSide();

  if (side == Side::Side::Buy) {
    if (m_bids.find(price) == m_bids.end()) {
      Level newBidLevel = Level(price, 0);
      LevelPointer newBidLevelPointer = std::make_shared<Level>(newBidLevel);
      m_bids[price] = newBidLevelPointer;
    }
    m_bids[price]->AddOrder(order);
  } else {
    if (m_asks.find(price) == m_asks.end()) {
      Level newAskLevel = Level(price, 0);
      LevelPointer newAskLevelPointer = std::make_shared<Level>(newAskLevel);
      m_asks[price] = newAskLevelPointer;
    }
    m_asks[price]->AddOrder(order);
  }
}

void OrderBook::CancelOrder(Order &order) {
  Price price = order.getPrice();
  Side::Side side = order.getSide();
  OrderID orderID = order.getOrderID();

  if (side == Side::Side::Buy) {
    m_bids[price]->CancelOrder(orderID);
    if (m_bids[price]->getQuantity() == 0)
      m_bids.erase(price);
  } else {
    m_asks[price]->CancelOrder(orderID);
    if (m_asks[price]->getQuantity() == 0)
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
    LevelPointer bestAskLevelPointer = (*m_asks.begin()).second;
    return (price >= bestAskLevelPointer->getPrice());
  } else {
    if (m_bids.empty())
      return false;
    LevelPointer bestBidLevelPointer = (*m_bids.begin()).second;
    return (price <= bestBidLevelPointer->getPrice());
  }
}

// https://www.learncpp.com/cpp-tutorial/stdoptional/
std::optional<Trade> OrderBook::MatchPotentialOrders() {

  if ((m_bids.empty() || m_asks.empty())) {
    return std::nullopt;
  }

  LevelPointer bestBidLevelPointer = (*m_bids.begin()).second;
  LevelPointer bestAskLevelPointer = (*m_asks.begin()).second;

  if (bestBidLevelPointer->getPrice() < bestAskLevelPointer->getPrice()) {
    return std::nullopt;
  }

  OrderPointer bestBidOrder = bestBidLevelPointer->getOrderList().front();
  OrderPointer bestAskOrder = bestAskLevelPointer->getOrderList().front();

  Quantity filledQuantity = std::min(bestBidOrder->getRemainingQuantity(),
                                     bestAskOrder->getRemainingQuantity());

  Price settlementPrice = bestAskLevelPointer->getPrice();

  bestBidOrder->FillPartially(filledQuantity);
  bestAskOrder->FillPartially(filledQuantity);
  bestBidLevelPointer->UpdateLevelQuantityPostMatch(filledQuantity);
  bestAskLevelPointer->UpdateLevelQuantityPostMatch(filledQuantity);

  if (bestBidOrder->isFullyFilled()) {
    OrderID bidOrderID = bestBidOrder->getOrderID();
    bestBidLevelPointer->removeMatchedOrder(bidOrderID);
    //    if (bestAskLevelPointer->getOrderList().empty())  equivalent condition
    //    for level deletion
    if (bestAskLevelPointer->getQuantity() == 0) {
      m_bids.erase(bestBidLevelPointer->getPrice());
    }
  }
  if (bestAskOrder->isFullyFilled()) {
    OrderID askOrderID = bestAskOrder->getOrderID();
    bestAskLevelPointer->removeMatchedOrder(askOrderID);
    if (bestAskLevelPointer->getQuantity() == 0) {
      m_asks.erase(bestAskLevelPointer->getPrice());
    }
  }

  OrderTraded bidTrade{bestBidOrder->getOrderID(), settlementPrice,
                       filledQuantity};
  OrderTraded askTrade{bestAskOrder->getOrderID(), settlementPrice,
                       filledQuantity};
  return Trade{bidTrade, askTrade};
}
