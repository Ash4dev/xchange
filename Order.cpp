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
             const std::string &activationTime,
             const std::string &deactivationTime)
    : m_symbol{symbol}, m_orderType{orderType}, m_side{side}, m_price{price},
      m_remQuantity{quantity} {

  m_timestamp = std::chrono::system_clock::now(); // time_point
  m_orderID =
      Order::encodeOrderID(m_timestamp, price, m_side == Side::Side::Buy);

  // expect time in dd-mm-yyyy hh:mm:ss format
  m_activateTime =
      ((activationTime == "") ? m_timestamp
                              : convertDateTimeToTimeStamp(activationTime));
  m_deactivateTime =
      ((deactivationTime == "") ? Constants::EndOfTime
                                : convertDateTimeToTimeStamp(deactivationTime));

  // market orders ensure execution at prevailing price
  if (orderType == OrderType::OrderType::Market ||
      orderType == OrderType::OrderType::MarketOnOpen ||
      orderType == OrderType::OrderType::MarketOnClose) {
    m_price = Constants::InvalidPrice;
  }
}

TimeStamp Order::convertDateTimeToTimeStamp(const std::string &dateTime) {
  // istringstream: input . -> string to buffer (cin/cout like)
  // stops when whitespace/fails
  std::istringstream ss(dateTime);

  // 24 hr format, 0-11 months, 2024->124 aka (2024-1900)
  // https://cplusplus.com/reference/ctime/tm/
  std::tm time;
  // parses string in given datetime fmt (opt to std::tm obj)
  ss >> std::get_time(&time, "%d-%m-%Y %H:%M:%S");
  if (ss.fail())
    throw std::runtime_error("Time could NOT be parsed");

  // returns no of secs from UNIX epoch (32 bit) -> 2038 till
  // TODO: check out and integrate __time64_t
  std::time_t tt = std::mktime(&time); // 0 param: current
  if (tt == -1)
    throw std::runtime_error("Failed to convert time to time_t");

  // Convert std::time_t -> std::chrono::system_clock::time_point
  return std::chrono::system_clock::from_time_t(tt);
}

// list initializer (runs before constructor)
Order::Order(const Order &other)
    : m_symbol{other.getSymbol()}, m_orderType{other.getOrderType()},
      m_side{other.getSide()}, m_price{other.getPrice()},
      m_remQuantity{other.getRemainingQuantity()},
      m_orderID{other.getOrderID()}, m_activateTime{other.getActivationTime()},
      m_deactivateTime{other.getDeactivationTime()} {}

OrderID Order::encodeOrderID(TimeStamp time, Price price, bool isBid) {
  assert(price * Constants::PriceMultiplier * 2 <= UINT32_MAX);
  std::uint32_t intPrice = price * Constants::PriceMultiplier;

  // https://stackoverflow.com/a/31553641
  // steady_clock: stopwatch, system_clock: timestamp
  // chrono: protects against unsafe type conversions (mostly manual)
  // TODO: instead of timestamp use random number seq 32 bits
  std::uint64_t timestamp = m_timestamp.time_since_epoch().count();

  // interpretation of encodeOrderID
  // MSB 32 bits: timestamp (uniqueness at ns scale)
  // next 31 bits store the Price to uint32_t
  // last stores side
  return (timestamp << 32) | (intPrice << 1) | isBid;
}
void Order::FillPartially(Quantity quantity) {
  assert(quantity <= m_remQuantity);
  m_remQuantity -= quantity;
}

// price-time priority algorithm (rank orders for matching)
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
