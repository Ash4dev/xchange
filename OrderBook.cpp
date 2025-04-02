#include "OrderBook.h"
#include "Level.h"
#include "Trade.h"
#include "utils/Constants.h"
#include "utils/alias/Fundamental.h"
#include "utils/alias/LevelRel.h"
#include "utils/alias/OrderRel.h"
#include "utils/enums/Side.h"

#include <cassert>
#include <cstdint>
#include <limits.h>
#include <memory>
#include <optional>

bool OrderBook::CancellationDueComparator::operator()(const OrderID &orderID1,
                                                      const OrderID &orderID2) {
  // when obj is defined as const, can't call non-const fn on it
  // to avoid any chance of modification to the original object
  Price price1 = decodePriceFromOrderID(orderID1);
  Side::Side side1 = decodeSideFromOrderID(orderID1);
  LevelPointer cancelL1 = orderBook->getLevelFromSideAndPrice(side1, price1);
  TimeStamp cancelT1 = cancelL1->getDeactivationTime(orderID1);

  Price price2 = decodePriceFromOrderID(orderID2);
  Side::Side side2 = decodeSideFromOrderID(orderID2);
  LevelPointer cancelL2 = orderBook->getLevelFromSideAndPrice(side2, price2);
  TimeStamp cancelT2 = cancelL2->getDeactivationTime(orderID2);

  return cancelT1 <= cancelT2;
}

bool OrderBook::AdditionDueComparator::operator()(const OrderID &orderID1,
                                                  const OrderID &orderID2) {
  Price price1 = decodePriceFromOrderID(orderID1);
  Side::Side side1 = decodeSideFromOrderID(orderID1);
  LevelPointer addL1 = orderBook->getLevelFromSideAndPrice(side1, price1);
  TimeStamp addT1 = addL1->getActivationTime(orderID1);

  Price price2 = decodePriceFromOrderID(orderID2);
  Side::Side side2 = decodeSideFromOrderID(orderID2);
  LevelPointer addL2 = orderBook->getLevelFromSideAndPrice(side2, price2);
  TimeStamp addT2 = addL2->getActivationTime(orderID2);

  return addT1 <= addT2;
}

Side::Side OrderBook::decodeSideFromOrderID(OrderID orderID) {
  return ((orderID & 0x1) ? Side::Side::Buy : Side::Side::Sell);
}

Price OrderBook::decodePriceFromOrderID(OrderID orderID) {
  orderID >>= 1;
  std::uint32_t scaledPrice = orderID & UINT_MAX;
  Price price = 1.0 * scaledPrice / Constants::PriceMultiplier;
  return price;
}

std::optional<Trade> OrderBook::AddOrder(Order &order) {
  if (order.getSymbol() != m_symbol) {
    return std::nullopt;
  }

  Price price = order.getPrice();
  Side::Side side = order.getSide();

  if (side == Side::Side::Buy) {
    if (m_bids.find(price) == m_bids.end()) {
      Level newBidLevel = Level(m_symbol, price, 0);
      LevelPointer newBidLevelPointer = std::make_shared<Level>(newBidLevel);
      m_bids[price] = newBidLevelPointer;
    }
    m_bids[price]->AddOrder(order);
  } else {
    if (m_asks.find(price) == m_asks.end()) {
      Level newAskLevel = Level(m_symbol, price, 0);
      LevelPointer newAskLevelPointer = std::make_shared<Level>(newAskLevel);
      m_asks[price] = newAskLevelPointer;
    }
    m_asks[price]->AddOrder(order);
  }
  return OrderBook::MatchPotentialOrders();
}

std::optional<Trade> OrderBook::CancelOrder(OrderID orderID) {
  Price price = OrderBook::decodePriceFromOrderID(orderID);
  Side::Side side = OrderBook::decodeSideFromOrderID(orderID);

  if (side == Side::Side::Buy) {
    m_bids[price]->CancelOrder(orderID);
    if (m_bids[price]->getQuantity() == 0)
      m_bids.erase(price);
  } else {
    m_asks[price]->CancelOrder(orderID);
    if (m_asks[price]->getQuantity() == 0)
      m_asks.erase(price);
  }
  return std::nullopt;
}

std::optional<Trade> OrderBook::ModifyOrder(OrderID orderID,
                                            Order &modifiedOrder) {
  OrderBook::CancelOrder(orderID);
  return OrderBook::AddOrder(modifiedOrder);
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
