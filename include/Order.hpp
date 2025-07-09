#pragma once

#include "utils/alias/Fundamental.hpp"
#include "utils/enums/OrderStatus.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <cassert>
#include <chrono>

class Order {
public:
  // various constructors
  Order() = default;
  Order(const Symbol symbol, const OrderType::OrderType orderType,
        const Side::Side side, const double price, const Quantity quantity,
        const ParticipantID &participantID,
        const std::string &activationTime = "",
        const std::string &deactivationTime = "");
  Order(const Order &other); // copy constructor

  // time functionality
  static std::string
  returnReadableTime(const std::chrono::system_clock::time_point &tt);
  static TimeStamp convertDateTimeToTimeStamp(const std::string &s);
  OrderID encodeOrderID(TimeStamp timestamp, Price price, bool isBid);

  bool operator<(const Order &other) const;
  bool operator==(const Order &other) const;

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
  ParticipantID getParticipantID() const { return m_participantID; }
  OrderStatus::OrderStatus getOrderStatus() const { return m_orderStatus; }

  void printTimeInfo() const;

  void setQuantity(Quantity newqty) { m_remQuantity = newqty; }
  void setPrice(Price newPrice) { m_price = newPrice; }
  void setOrderType(OrderType::OrderType newType) { m_orderType = newType; }
  void setActivationTime(TimeStamp newtime) { m_activateTime = newtime; }
  void setDeactivationTime(TimeStamp newtime) { m_deactivateTime = newtime; }
  void setOrderStatus(OrderStatus::OrderStatus newOrderStatus) {
    m_orderStatus = newOrderStatus;
  }

private:
  Symbol m_symbol;
  OrderType::OrderType m_orderType;
  Side::Side m_side;
  Price m_price;
  Quantity m_remQuantity;
  ParticipantID m_participantID;
  TimeStamp m_timestamp;
  OrderID m_orderID;
  TimeStamp m_activateTime;
  TimeStamp m_deactivateTime;
  OrderStatus::OrderStatus m_orderStatus;
};
