#include "utils/helpers/HelperFunctions.hpp"
#include <iomanip>

// convert str to str optional
std::optional<std::string> parseOptionalString(const std::string &str) {
  if (str == "-" || str.empty())
    return std::nullopt;
  else
    return str;
}

// parses string in given datetime fmt (opt to std::tm obj)
TimeStamp parseTimeStamp(const std::string &dateTime) {
  std::istringstream ss(dateTime);

  std::tm time = {}; // always init fully (earlier garbage value of 13)
  ss >> std::get_time(&time, "%d-%m-%Y/%H:%M:%S");
  if (ss.fail())
    throw std::runtime_error("Time could NOT be parsed");

  time.tm_isdst = -1; // let sys decide if daylight saving applicable

  std::time_t tt = std::mktime(&time);
  if (tt == -1)
    throw std::runtime_error("Failed to convert time to time_t");

  // Convert std::time_t -> std::chrono::system_clock::time_point
  return std::chrono::system_clock::from_time_t(tt);
}

Actions::Actions parseAction(const std::string &str) {
  if (str == "Add")
    return Actions::Actions::Add;
  if (str == "Cancel")
    return Actions::Actions::Cancel;
  if (str == "Modify")
    return Actions::Actions::Modify;
  throw std::runtime_error("Unknown Action: " + str);
}

Side::Side parseSide(const std::string &str) {
  if (str == "Buy")
    return Side::Side::Buy;
  if (str == "Sell")
    return Side::Side::Sell;
  throw std::runtime_error("Unknown Side: " + str);
}

OrderType::OrderType parseOrderType(const std::string &typeString) {
  if (typeString == "Market") {
    return OrderType::OrderType::Market;
  } else if (typeString == "FillOrKill") {
    return OrderType::OrderType::FillOrKill;
  } else if (typeString == "ImmediateOrCancel") {
    return OrderType::OrderType::ImmediateOrCancel;
  } else if (typeString == "GoodAfterTime") {
    return OrderType::OrderType::GoodAfterTime;
  } else if (typeString == "GoodForDay") {
    return OrderType::OrderType::GoodForDay;
  } else if (typeString == "GoodTillDate") {
    return OrderType::OrderType::GoodTillDate;
  } else if (typeString == "AllOrNone") {
    return OrderType::OrderType::AllOrNone;
  } else if (typeString == "GoodTillCancel") {
    return OrderType::OrderType::GoodTillCancel;
  } else if (typeString == "MarketOnOpen") {
    return OrderType::OrderType::MarketOnOpen;
  } else if (typeString == "MarketOnClose") {
    return OrderType::OrderType::MarketOnClose;
  }
  throw std::runtime_error("Unknown OrderType: " + typeString);
}
