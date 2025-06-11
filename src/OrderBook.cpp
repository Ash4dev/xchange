#include "include/OrderBook.hpp"
#include "include/Level.hpp"
#include "include/Trade.hpp"

#include "utils/alias/Fundamental.hpp"
#include "utils/alias/LevelRel.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <cassert>
#include <cstdint>
#include <limits.h>
#include <memory>
#include <optional>

Side::Side OrderBook::decodeSideFromOrderID(const OrderID orderID) {
  return ((orderID & 0x1)
              ? Side::Side::Buy
              : Side::Side::Sell); // side is the last bit of orderID
}

Price OrderBook::decodePriceFromOrderID(const OrderID orderID) {
  // last bit is not imp for price information
  // & with all 1s retains on bits (can't store floats: so int)
  // this mask is not the same as UINT32_MAX
  std::uint32_t price = (orderID >> 1) & ((static_cast<OrderID>(1) << 31) - 1);
  return price;
}

// optional since trade may/may not be possible C++ 17 feature
// essentially returns 2 values: bool (success/failure) & return val (result)
// optional is syntactic sugar, no need of ugly manual implementation

// cannot add using orderID before it has been added to the orderbook (silly
// doubt)
std::optional<Trade> OrderBook::AddOrder(Order &order) {
  if (order.getSymbol() !=
      m_symbol) {        // order does not belong to this OrderBook
    return std::nullopt; //  failure indicator
  }

  // handling any market order matching (timing by preprocessor)
  if (order.getOrderType() == OrderType::OrderType::Market ||
      order.getOrderType() == OrderType::OrderType::MarketOnClose ||
      order.getOrderType() == OrderType::OrderType::MarketOnOpen) {

    if (order.getSide() == Side::Side::Buy) {
      if (!m_asks.empty()) {
        // make capable enough to match with the worst ask
        // asks above it get fulfilled (may not last till worst)
        Price newPrice = (*m_asks.rbegin()).first;
        order.setPrice(newPrice);
      }
    } else {
      if (!m_bids.empty()) {
        Price newPrice = (*m_bids.rbegin()).first; // same logic as bids
        order.setPrice(newPrice);
      }
    }
    // the order becomes a limit order now
    order.setOrderType(OrderType::OrderType::GoodTillCancel);
  }

  Price price = order.getPrice();
  Side::Side side = order.getSide();

  if (side == Side::Side::Buy) { // if a bid placed, check on ask (vice-versa)
    if (m_bids.find(price) == m_bids.end()) { // if price level not exists
      Level newBidLevel = Level(m_symbol, price, 0); // create level & pointer
      LevelPointer newBidLevelPointer = std::make_shared<Level>(newBidLevel);
      m_bids[price] = newBidLevelPointer; // store level on bids side
    }
    m_bids[price]->AddOrder(order); // add order to the level
  } else {
    if (m_asks.find(price) == m_asks.end()) {
      Level newAskLevel = Level(m_symbol, price, 0);
      LevelPointer newAskLevelPointer = std::make_shared<Level>(newAskLevel);
      m_asks[price] = newAskLevelPointer;
    }
    m_asks[price]->AddOrder(order);
  }
  return OrderBook::MatchPotentialOrders(); // check if match possible
}

// orderID must exist for existing order, since only it can be deleted
std::optional<Trade> OrderBook::CancelOrder(OrderID orderID) {
  Price price = OrderBook::decodePriceFromOrderID(orderID);
  Side::Side side = OrderBook::decodeSideFromOrderID(orderID);

  // debugging: printed price & trade, before & after: ol size, m_x size

  if (side == Side::Side::Buy) {
    if (m_bids.count(price) == 0) {
      std::cout << "no such bid level w price " << price << std::endl;
      return std::nullopt;
    }
    m_bids[price]->CancelOrder(orderID); // cancel it
    if (m_bids[price]->getQuantity() ==
        0) { // no more volume on level, delete it
      m_bids.erase(price);
    }

  } else {
    if (m_asks.count(price) == 0) {
      std::cout << "no such ask level w price " << price << std::endl;
      return std::nullopt;
    }
    m_asks[price]->CancelOrder(orderID);
    if (m_asks[price]->getQuantity() == 0) {
      m_asks.erase(price);
    }
  }

  // cancellation cannot yield a match: if could be matched, would have
  // ends returns std::nullopt
  return std::nullopt;
}

std::optional<Trade> OrderBook::ModifyOrder(OrderID orderID,
                                            Order &modifiedOrder) {
  OrderBook::CancelOrder(orderID);
  return OrderBook::AddOrder(modifiedOrder);
}

// act as check before matching TODO: where the hell is it used??
bool OrderBook::CanMatchOrder(Side::Side side, Price price) const {
  // ternary operator fails for incompatible types
  // m_bids and m_asks are incompatible types: comparators

  if (side == Side::Side::Buy) { // if a bid, match w asks (vice-versa)
    if (m_asks.empty())          // no ask no match
      return false;
    LevelPointer bestAskLevelPointer =
        (*m_asks.begin()).second; // find best ask
    return (price >=
            bestAskLevelPointer->getPrice()); // bid >= ask (trade occur)
  } else {
    if (m_bids.empty())
      return false;
    LevelPointer bestBidLevelPointer = (*m_bids.begin()).second;
    return (price <= bestBidLevelPointer->getPrice());
  }
}

// https://www.learncpp.com/cpp-tutorial/stdoptional/
std::optional<Trade> OrderBook::MatchPotentialOrders() {

  if ((m_bids.empty() || m_asks.empty())) { // bids & asks both must for trade
    return std::nullopt;                    // failure indicator
  }

  // obtain best levels on each side
  LevelPointer bestBidLevelPointer = (*m_bids.begin()).second;
  LevelPointer bestAskLevelPointer = (*m_asks.begin()).second;

  if (bestBidLevelPointer->getPrice() <
      bestAskLevelPointer->getPrice()) { // bidp >= askp for trade
    return std::nullopt;
  }

  // get first orders on each sides best level
  OrderPointer bestBidOrder = bestBidLevelPointer->getOrderList().front();
  OrderPointer bestAskOrder = bestAskLevelPointer->getOrderList().front();

  // min of both qty can only be filled
  Quantity filledQuantity = std::min(bestBidOrder->getRemainingQuantity(),
                                     bestAskOrder->getRemainingQuantity());

  // TODO: confirm correctness? askprice or bidprice
  // ask: buyer spends less than willing & sellers get desired
  // bid: buyers spends as much as willing & sellers get more
  // avg of bid and ask since don't know aggressor & fair
  double settlementPrice = (1.0 * bestAskLevelPointer->getPrice()) / 100;
  // Price settlementPrice =
  // (bestBidLevelPointer->getPrice() + bestAskLevelPointer->getPrice()) / 2;
  ;

  // update the remaining volume of orders
  bestBidOrder->FillPartially(filledQuantity);
  bestAskOrder->FillPartially(filledQuantity);
  // update volumes of levels of both sides
  bestBidLevelPointer->UpdateLevelQuantityPostMatch(filledQuantity);
  bestAskLevelPointer->UpdateLevelQuantityPostMatch(filledQuantity);

  // remove order from level if no remaining Quantity
  if (bestBidOrder->isFullyFilled()) {
    OrderID bidOrderID = bestBidOrder->getOrderID();
    bestBidLevelPointer->removeMatchedOrder(bidOrderID); // remove order

    //    if (bestBidLevelPointer->getOrderList().empty())  equivalent condition
    if (bestBidLevelPointer->getQuantity() == 0) { // if level empty, remove it
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

  // store trade information
  OrderTraded bidTrade{bestBidOrder->getOrderID(), settlementPrice,
                       filledQuantity};
  OrderTraded askTrade{bestAskOrder->getOrderID(), settlementPrice,
                       filledQuantity};
  return Trade{bidTrade, askTrade};
}

void OrderBook::printOrderBookState(const std::string &message) {
  std::cout << message << std::endl;

  std::cout << "----------BID------------" << std::endl;
  for (auto &ele : OrderBook::getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev->getPrice() << " " << lev->getQuantity() << " "
              << lev->getOrderList().size() << std::endl;
  }
  std::cout << "----------ASK------------" << std::endl;

  std::cout << "ASK" << std::endl;
  for (auto &ele : OrderBook::getAskLevels()) {
    auto &lev = ele.second;
    std::cout << lev->getPrice() << " " << lev->getQuantity() << " "
              << lev->getOrderList().size() << std::endl;
  }
  std::cout << "----------DONE------------" << std::endl;
}
