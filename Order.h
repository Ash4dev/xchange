#pragma once

#include <cassert>
#include <cstdint>
#include <stdexcept>

// #include <chrono>
// #include <ctime>   // for std::tm, mktime
// #include <iomanip> // for std::get_time (if parsing from string)
// #include <sstream> // for std::istringstream (if parsing from string)
//
#include "utils/Constants.h"
#include "utils/alias/Fundamental.h"
#include "utils/enums/OrderTypes.h"
#include "utils/enums/Side.h"

class Order {
public:
  Order() = default;
  Order(const Symbol symbol, const OrderType::OrderType orderType,
        const Side::Side side, const Price price, const Quantity quantity,
        const std::string &activationTime, const std::string &deactivationTime);

  // copy constructor
  Order(const Order &other);

  TimeStamp convertDateTimeToTimeStamp(const std::string &s);
  OrderID encodeOrderID(TimeStamp timestamp, Price price, bool isBid);
  bool operator<(const Order &other) const;

  void FillPartially(Quantity quantity);
  bool isFullyFilled();

  // const member fns: can't change val of data member
  Symbol getSymbol() const { return m_symbol; }
  OrderType::OrderType getOrderType() const { return m_orderType; }
  Side::Side getSide() const { return m_side; }
  Price getPrice() const { return m_price; }
  Quantity getRemainingQuantity() const { return m_remQuantity; }
  OrderID getOrderID() const { return m_orderID; }
  TimeStamp getOrderTime() const { return m_timestamp; }
  TimeStamp getActivationTime() const { return m_activateTime; }
  TimeStamp getDeactivationTime() const { return m_deactivateTime; }

private:
  Symbol m_symbol;
  OrderType::OrderType m_orderType;
  Side::Side m_side;
  Price m_price;
  Quantity m_remQuantity;
  TimeStamp m_timestamp;
  OrderID m_orderID;
  TimeStamp m_activateTime;
  TimeStamp m_deactivateTime;
};
