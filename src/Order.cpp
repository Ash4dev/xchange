#include "include/Order.hpp"
#include "utils/Constants.hpp"
#include "utils/alias/Fundamental.hpp"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>

#define PRICE_MULTIPLIER 100

std::string
Order::returnReadableTime(const std::chrono::system_clock::time_point &tt) {
  std::time_t tamatch = std::chrono::system_clock::to_time_t(tt);
  std::tm *ptamatch = std::localtime(&tamatch); // local time conversion

  std::stringstream ss;
  ss << std::put_time(ptamatch, "%Y-%m-%d %H:%M:%S");
  std::string ans = ss.str();
  return ans;
}

void Order::printTimeInfo() const {
  std::cout << "activate: " << returnReadableTime(m_activateTime)
            << " deactivate: " << returnReadableTime(m_deactivateTime)
            << std::endl;
}

Order::Order(const Symbol symbol, OrderType::OrderType orderType,
             Side::Side side, double price, Quantity quantity,
             const std::string &activationTime,
             const std::string &deactivationTime)
    : m_symbol{symbol}, m_orderType{orderType}, m_side{side},
      m_price{static_cast<int>(price * PRICE_MULTIPLIER)},
      m_remQuantity{quantity} {

  // static_cast: purely compile time explicit inbuilt/custom cast operator
  // no runtime check -> typecast to parent class, but access child method
  // above problematic -> solution: dynamic_cast
  // keep sure all values are in int to avoid round off, precision errors etc
  m_timestamp = std::chrono::system_clock::now(); // time_point
  m_orderID =
      Order::encodeOrderID(m_timestamp, m_price, m_side == Side::Side::Buy);

  // expect time in dd-mm-yyyy hh:mm:ss format
  m_activateTime =
      ((activationTime == "") ? m_timestamp
                              : convertDateTimeToTimeStamp(activationTime));
  m_deactivateTime =
      ((deactivationTime == "") ? Constants::EndOfTime
                                : convertDateTimeToTimeStamp(deactivationTime));

  // market orders ensure execution at prevailing price
  // if (orderType == OrderType::OrderType::Market ||
  //     orderType == OrderType::OrderType::MarketOnOpen ||
  //     orderType == OrderType::OrderType::MarketOnClose) {
  //   m_price = Constants::InvalidPrice;
  // }
}

TimeStamp Order::convertDateTimeToTimeStamp(const std::string &dateTime) {
  // istringstream: input . -> string to buffer (cin/cout like)
  // stops when whitespace/fails
  std::istringstream ss(dateTime);

  // 24 hr format, 0-11 months, 2024->124 aka (2024-1900)
  // https://cplusplus.com/reference/ctime/tm/
  std::tm time = {}; // always init fully (earlier garbage value of 13)
  // parses string in given datetime fmt (opt to std::tm obj)
  ss >> std::get_time(&time, "%d-%m-%Y %H:%M:%S");
  if (ss.fail())
    throw std::runtime_error("Time could NOT be parsed");

  // cause of sparring execution (13 somehow)
  time.tm_isdst = -1; // let sys decide if daylight saving applicable
  // std::cout << "Parsed tm: "
  //           << "Day=" << time.tm_mday << ", "
  //           << "Month=" << time.tm_mon + 1 << ", "
  //           << "Year=" << time.tm_year + 1900 << ", "
  //           << "Hour=" << time.tm_hour << ", "
  //           << "Min=" << time.tm_min << ", "
  //           << "Sec=" << time.tm_sec << ", "
  //           << "DST: " << time.tm_isdst << std::endl;

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

OrderID Order::encodeOrderID(TimeStamp time, Price intPrice, bool isBid) {

  // https://stackoverflow.com/a/31553641
  // steady_clock: stopwatch, system_clock: timestamp
  // chrono: protects against unsafe type conversions (mostly manual)

  // timestamp at ns scale provides enough uniqueness
  // hence no random number used in orderID
  std::uint64_t timestamp = time.time_since_epoch().count();

  // interpretation of encodeOrderID
  // bit 63-32: timestamp (uniqueness at ns scale)
  // bit 31-1: store the Price to uint32_t
  // bit 0: stores side

  // put whole expressions in parenthesis (operator precendence screwed us)
  // determined by prinitng pre-encoding & post-encoding version of each param
  OrderID id = 0;
  id |= ((static_cast<OrderID>(timestamp)) << 32);
  id |= ((static_cast<OrderID>(intPrice)) << 1);
  id |= ((isBid) ? static_cast<OrderID>(1) : static_cast<OrderID>(0));
  return id;
  // return ((timestamp << 31) | (intPrice << 1) | isBid);
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

bool Order::operator==(const Order &other) const {
  return m_symbol == other.m_symbol && m_orderType == other.m_orderType &&
         m_side == other.m_side && m_price == other.m_price &&
         m_remQuantity == other.m_remQuantity &&
         m_timestamp == other.m_timestamp && m_orderID == other.m_orderID &&
         m_activateTime == other.m_activateTime &&
         m_deactivateTime == other.m_deactivateTime;
}

bool Order::isFullyFilled() { return (getRemainingQuantity() == 0); }
