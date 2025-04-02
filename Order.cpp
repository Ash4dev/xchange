#include "Order.h"
#include "utils/Constants.h"
#include "utils/alias/Fundamental.h"

#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

Order::Order(const Symbol symbol, OrderType::OrderType orderType,
             Side::Side side, Price price, Quantity quantity,
             const std::string &activationTime = "",
             const std::string &deactivationTime = "")
    : m_symbol{symbol}, m_orderType{orderType}, m_side{side}, m_price{price},
      m_remQuantity{quantity} {
  m_timestamp = std::chrono::system_clock::now();
  m_orderID =
      Order::encodeOrderID(m_timestamp, price, m_side == Side::Side::Buy);
  m_activateTime =
      ((activationTime == "") ? m_timestamp
                              : convertDateTimeToTimeStamp(activationTime));
  m_deactivateTime =
      ((deactivationTime == "") ? Constants::EndOfTime
                                : convertDateTimeToTimeStamp(deactivationTime));
  if (orderType == OrderType::OrderType::Market ||
      orderType == OrderType::OrderType::MarketOnOpen ||
      orderType == OrderType::OrderType::MarketOnClose) {
    m_price = Constants::InvalidPrice;
  }
}

TimeStamp Order::convertDateTimeToTimeStamp(const std::string &dateTime) {
  std::tm time;

  std::istringstream ss(dateTime);
  ss >> std::get_time(&time, "%d-%m-%Y %H:%M:%S");
  if (ss.fail())
    throw std::runtime_error("Time could NOT be parsed");

  std::time_t tt = std::mktime(&time);
  if (tt == -1)
    throw std::runtime_error("Failed to convert time to time_t");

  // Convert std::time_t -> std::chrono::system_clock::time_point
  return std::chrono::system_clock::from_time_t(tt);
}

Order::Order(const Order &other)
    : m_symbol{other.getSymbol()}, m_orderType{other.getOrderType()},
      m_side{other.getSide()}, m_price{other.getPrice()},
      m_remQuantity{other.getRemainingQuantity()},
      m_orderID{other.getOrderID()}, m_activateTime{other.getActivationTime()},
      m_deactivateTime{other.getDeactivationTime()} {}

OrderID Order::encodeOrderID(TimeStamp time, Price price, bool isBid) {
  assert(price * Constants::PriceMultiplier * 2 <= UINT32_MAX);
  std::uint64_t timestamp = m_timestamp.time_since_epoch().count();
  std::uint32_t intPrice = price * Constants::PriceMultiplier;
  // MSB 32 bits: timestamp (uniqueness at ns milisec scale)
  // next 31 bits store the Price to uint32_t
  // last stores side
  return (timestamp << 32) | (intPrice << 1) | isBid;
}

void Order::FillPartially(Quantity quantity) {
  assert(quantity <= m_remQuantity);
  m_remQuantity -= quantity;
}

bool Order::operator<(const Order &other) const {
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

bool Order::isFullyFilled() { return (getRemainingQuantity() == 0); }
