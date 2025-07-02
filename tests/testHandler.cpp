#include "tests/testHandler.hpp"

#include <chrono>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

//////////////////////////////////////////
//// FREE FUNCTIONS FWD DECLARATIONS/////
////////////////////////////////////////

std::vector<std::string> splitString(const std::string &str, char delimiter);
std::string trimNonDataString(const std::string &line);
bool containsNoDigits(const std::string &line);
std::string trimDataString(const std::string &line);

TimeStamp parseTimeStamp(const std::string &dateTime);
Actions::Actions parseAction(const std::string &str);
OrderType::OrderType parseOrderType(const std::string &typeString);
Side::Side parseSide(const std::string &str);

template <typename T>
std::optional<T> parseOptionalNumeric(const std::string &str);
std::optional<std::string> parseOptionalString(const std::string &str);

//////////////////////////////////////////
//// CORE TEST HANDLING FUNCTIONALITY////
////////////////////////////////////////

void TestHandler::parseTestFile(const std::string &testFilePath) {
  std::ifstream testFile(testFilePath); // string_view fails
  while (!testFile.is_open()) {
    throw std::runtime_error("Cannot open file: " + testFilePath);
    return;
  }

  std::string line;
  while (std::getline(testFile, line)) {
    if (line.length() > 0 && line[0] == '-')
      line = trimNonDataString(line);

    if (line.find("PendingOrderThreshold") != std::string::npos) {
      auto pos = line.find(":");
      if (pos != std::string::npos) {
        m_MAX_PENDING_ORDER_THRESHOLD =
            static_cast<std::size_t>(std::stoull(line.substr(pos + 2)));
      }
    } else if (line.find("PendingDurationThreshold") != std::string::npos) {
      auto pos = line.find(":");
      if (pos != std::string::npos) {
        m_MAX_PENDING_DURATION_THRESHOLD =
            static_cast<std::chrono::milliseconds>(
                std::stoull(line.substr(pos + 2)));
      }
    } else if (line.find("INCOMING ORDERS") != std::string::npos) {
      parseUpdate(testFile);
    } else if (line.find("PREPROCESSOR") != std::string::npos) {
      parsePreprocessorResults(testFile);
    } else if (line.find("ORDERBOOK") != std::string::npos) {
      parseOrderbookResults(testFile);
    } else if (line.find("TRADES") != std::string::npos) {
      parseTradeResults(testFile);
    } else {
      continue;
    }
  }
}

// void TestHandler::performUpdates() {
//   for (const auto &update : m_finalUpdates) {
//     TestHandler::performUpdate();
//   }
// }
//
// bool TestHandler::verifyResults(const SymbolInfoPointer &symbolInfo) {
//   return true;
// }

std::size_t TestHandler::getPreProcessorArgs() {
  return m_MAX_PENDING_ORDER_THRESHOLD;
}
std::vector<Update> TestHandler::getUpdates() { return m_finalUpdates; }
std::vector<PreProcessorResult> TestHandler::getPreProcessorResults() {
  return m_preResults;
}
std::vector<OrderBookResult> TestHandler::getOrderBookResults() {
  return m_obResults;
}
std::vector<TradeResult> TestHandler::getTradesResults() { return m_trResults; }

//////////////////////////////////////////
//// TESTHELPER FUNCTIONS FOR PARSING //
////////////////////////////////////////

void TestHandler::parseUpdate(std::ifstream &testFile) {
  std::string line;
  while (std::getline(testFile, line)) {
    if (line.length() > 0 && line[0] == '-') {
      if (trimNonDataString(line).find("EXPECTED RESULT") != std::string::npos)
        return;
      continue;
    } else if (containsNoDigits(line))
      continue;
    else
      line = trimDataString(line);

    std::vector<std::string> tokens = splitString(line, ' ');
    Update upd{};
    upd.updateCount = std::stoul(tokens[0]);
    upd.action = parseAction(tokens[1]);
    upd.symbol = parseOptionalString(tokens[2]);
    upd.orderType = tokens[3] == "-"
                        ? std::nullopt
                        : std::make_optional(parseOrderType(tokens[3]));
    upd.side = tokens[4] == "-" ? std::nullopt
                                : std::make_optional(parseSide(tokens[4]));
    upd.price = parseOptionalNumeric<Price>(tokens[5]);
    upd.quantity = parseOptionalNumeric<Quantity>(tokens[6]);
    upd.activationTime = tokens[7] == "-"
                             ? std::nullopt
                             : std::make_optional(parseTimeStamp(tokens[7]));
    upd.deactivationTime = tokens[8] == "-"
                               ? std::nullopt
                               : std::make_optional(parseTimeStamp(tokens[8]));
    upd.participantID = parseOptionalString(tokens[9]);
    m_finalUpdates.push_back(upd);
  }
}

void TestHandler::parsePreprocessorResults(std::ifstream &testFile) {
  std::string line;
  while (std::getline(testFile, line)) {
    if (line.length() > 0 && line[0] == '-') {
      if (trimNonDataString(line).find("PREPROCESSOR END") != std::string::npos)
        return;
      continue;
    } else if (containsNoDigits(line))
      continue;
    else
      line = trimDataString(line);

    std::vector<std::string> tokens = splitString(line, ' ');
    PreProcessorResult r;
    r.symbol = tokens[0];
    r.side = parseSide(tokens[1]);
    r.action = parseAction(tokens[2]);
    r.price = static_cast<Price>(std::stoll(tokens[3]));
    r.quantity = static_cast<Quantity>(std::stoll(tokens[4]));
    r.participantID = tokens[5];
    m_preResults.push_back(r);
  }
}

void TestHandler::parseOrderbookResults(std::ifstream &testFile) {
  std::string line;
  while (std::getline(testFile, line)) {
    if (line.length() > 0 && line[0] == '-') {
      if (trimNonDataString(line).find("ORDERBOOK END") != std::string::npos)
        return;
      continue;
    } else if (containsNoDigits(line))
      continue;
    else
      line = trimDataString(line);

    std::vector<std::string> tokens = splitString(line, ' ');
    OrderBookResult r;
    r.symbol = tokens[0];
    r.side = parseSide(tokens[1]);
    r.price = static_cast<Price>(std::stoll(tokens[2]));
    r.quantity = static_cast<Quantity>(std::stoll(tokens[3]));
    r.orderListSize = std::stoull(tokens[4]);
    m_obResults.push_back(r);
  }
}

void TestHandler::parseTradeResults(std::ifstream &testFile) {
  std::string line;
  while (std::getline(testFile, line)) {
    if (line.length() > 0 && line[0] == '-') {
      if (trimNonDataString(line).find("TRADES END") != std::string::npos)
        return;
      continue;
    } else if (containsNoDigits(line))
      continue;
    else
      line = trimDataString(line);

    std::vector<std::string> tokens = splitString(line, ' ');
    TradeResult r;
    r.symbol = tokens[0];
    r.price = static_cast<Price>(std::stoll(tokens[1]));
    r.quantity = static_cast<Quantity>(std::stoll(tokens[2]));
    r.buyerID = tokens[3];
    r.sellerID = tokens[4];
    m_trResults.push_back(r);
  }
}

//////////////////////////////////////////
//// FREE HELPER FUNCTIONS FOR PARSING //
////////////////////////////////////////

std::vector<std::string> splitString(const std::string &str, char delimiter) {
  std::vector<std::string> tokens;
  std::istringstream iss(str); // Create an input string stream from the string
  std::string token;

  // Extract tokens using getline with the specified delimiter
  while (std::getline(iss, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

std::string trimNonDataString(const std::string &input) {
  std::string result = "";
  for (char c : input) {
    if (c != '-')
      result += c;
  }
  return result;
}

bool containsNoDigits(const std::string &str) {
  for (char ch : str) {
    if (isdigit(ch)) {
      return false;
    }
  }
  return true;
}

// trim down the data field of waste chars like " ", "\" etc
// https://onlinegdb.com/6Rq1vatsZ
std::string trimDataString(const std::string &input) {
  std::string result = "";
  bool spaceFound = false;
  bool charFirstSeen = false;

  for (char c : input) {
    if (c == '|')
      continue;
    else if (std::isspace(static_cast<unsigned char>(c))) {
      spaceFound = true;
    } else {
      if (spaceFound && charFirstSeen) {
        result += ' ';
      }
      result += c;
      charFirstSeen = true;
      spaceFound = false;
    }
  }
  return result;
}

// convert str types to T optional through long long
template <typename T>
std::optional<T> parseOptionalNumeric(const std::string &str) {
  if (str == "-" || str.empty())
    return std::nullopt;
  else
    return static_cast<T>(std::stoll(str)); // str -> ll -> T
}
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
