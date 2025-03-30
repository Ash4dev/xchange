#pragma once

#include <cassert>
#include <chrono>
#include <stdexcept>

#include "utils/Constants.h"
#include "utils/alias/Fundamental.h"
#include "utils/enums/OrderTypes.h"
#include "utils/enums/Side.h"

class Order {
public:
  Order() = default;
  // all following parameters must have default params as well
  Order(Symbol symbol, OrderType::OrderType orderType, Side::Side side,
        Price price, Quantity quantity)
      : m_symbol{symbol}, m_orderType{orderType}, m_side{side}, m_price{price},
        m_remQuantity{quantity} {
    m_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    m_orderID = m_timestamp; // check this

    if (orderType == OrderType::OrderType::Market ||
        orderType == OrderType::OrderType::MarketOnOpen ||
        orderType == OrderType::OrderType::MarketOnClose) {
      m_price = Constants::InvalidPrice;
    }
  }

  // copy constructor
  Order(const Order &other)
      : m_symbol{other.getSymbol()}, m_orderType{other.getOrderType()},
        m_side{other.getSide()}, m_price{other.getPrice()},
        m_remQuantity{other.getRemainingQuantity()},
        m_orderID{other.getOrderID()} {}

  void FillPartially(Quantity quantity) {
    assert(quantity <= m_remQuantity);
    m_remQuantity -= quantity;
  }

  bool operator<(const Order &other) const {
    if (m_side == other.getSide()) {
      if (m_side == Side::Side::Buy && m_price != other.getPrice())
        return m_price > other.getPrice();
      else if (m_side == Side::Side::Sell && m_price != other.getPrice())
        return m_price < other.getPrice();
    }

    if (m_timestamp != other.getOrderTime())
      return m_timestamp < other.getOrderTime();
    return m_remQuantity > other.getRemainingQuantity();
  }

  bool isFullyFilled() { return (getRemainingQuantity() == 0); }

  Symbol getSymbol() const { return m_symbol; }
  // const member fns: can't change val of data member
  OrderType::OrderType getOrderType() const { return m_orderType; }
  Side::Side getSide() const { return m_side; }
  Price getPrice() const { return m_price; }
  Quantity getRemainingQuantity() const { return m_remQuantity; }
  OrderID getOrderID() const { return m_orderID; }
  TimeStamp getOrderTime() const { return m_timestamp; }

private:
  Symbol m_symbol;
  OrderType::OrderType m_orderType;
  Side::Side m_side;
  Price m_price;
  Quantity m_remQuantity;
  TimeStamp m_timestamp;
  OrderID m_orderID;
};
