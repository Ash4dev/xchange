#include "tests/testHandler.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/enums/Side.hpp"

#include <iostream>
#include <string>
#include <unistd.h>

std::string
returnReadableTime(const std::chrono::system_clock::time_point &tt) {
  std::time_t tamatch = std::chrono::system_clock::to_time_t(tt);
  std::tm *ptamatch = std::localtime(&tamatch); // local time conversion

  std::stringstream ss;
  ss << std::put_time(ptamatch, "%Y-%m-%d %H:%M:%S");
  std::string ans = ss.str();
  return ans;
}

void printTimeInfo(TimeStamp m_activateTime) {
  std::cout << "activate: " << returnReadableTime(m_activateTime) << std::endl;
}

std::ostream &operator<<(std::ostream &os, const Update &update) {
  os << "Update {\n"
     << "  updateCount: " << update.updateCount << "\n"
     << "  action: " << static_cast<int>(update.action) << "\n";

  // Helper lambda to print optional fields
  auto printOptional = [&os](const auto &opt, const std::string &name) {
    if (opt) {
      os << "  " << name << ": " << *opt << "\n";
    } else {
      os << "  " << name << ": (null)\n";
    }
  };

  printOptional(update.symbol, "symbol");
  printOptional(update.orderType, "orderType");
  printOptional(update.side, "side");
  printOptional(update.price, "price");
  printOptional(update.quantity, "quantity");
  printOptional(update.participantID, "participantID");

  os << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const PreProcessorResult &res) {
  os << "PreProcessorResult {\n"
     << "  symbol: " << res.symbol << "\n"
     << "  side: " << static_cast<int>(res.side) << "\n"
     << "  action: " << static_cast<int>(res.action) << "\n"
     << "  price: " << res.price << "\n"
     << "  quantity: " << res.quantity << "\n"
     << "  participantID: " << res.participantID << "\n"
     << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const OrderBookResult &res) {
  os << "OrderBookResult {\n"
     << "  side: " << static_cast<int>(res.side) << "\n"
     << "  symbol: " << res.symbol << "\n"
     << "  price: " << res.price << "\n"
     << "  quantity: " << res.quantity << "\n"
     << "  orderListSize: " << res.orderListSize << "\n"
     << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const TradeResult &res) {
  os << "TradeResult {\n"
     << "  symbol: " << res.symbol << "\n"
     << "  price: " << res.price << "\n"
     << "  quantity: " << res.quantity << "\n"
     << "  buyerID: " << res.buyerID << "\n"
     << "  sellerID: " << res.sellerID << "\n"
     << "}";
  return os;
}

std::string to_string(Side::Side side) {
  switch (side) {
  case Side::Side::Buy:
    return "BUY";
  case Side::Side::Sell:
    return "SELL";
  default:
    return std::to_string(static_cast<int>(side));
  }
}

int main() {
  // (base) ➜  xchange git:(main) ✗ g++ -I. -Itests/ tests/test-check.cpp
  // tests/testHandler.cpp -o tests/a.out (base)
  // ➜  xchange git:(main) ✗ ./tests/a.out
  TestHandler th = TestHandler();
  std::string path =
      "/home/ashron/Desktop/expts/xchange/tests/sample/TestCaseFormat.txt";
  th.parseTestFile(path);

  th.getPreProcessorArgs();

  auto updates = th.getUpdates();
  for (auto ele : updates) {
    std::cout << ele << std::endl;
    if (ele.activationTime.has_value())
      printTimeInfo(ele.activationTime.value());
    if (ele.deactivationTime.has_value())
      printTimeInfo(ele.deactivationTime.value());
  }

  auto pre = th.getPreProcessorResults();
  for (auto ele : pre) {
    std::cout << ele << std::endl;
  }

  auto ob = th.getOrderBookResults();
  for (auto ele : ob) {
    std::cout << ele << std::endl;
  }

  std::cout << ob.size() << std::endl;

  auto tr = th.getTradesResults();
  for (auto ele : tr) {
    std::cout << ele << std::endl;
  }

  // install g-test using sudo apt-get install libgtest-dev
  // #include <gtest/gtest.h>
  // TEST(LOL, LOL1) { ASSERT_EQ(1, 1); }
  // testing::InitGoogleTest();
  // return RUN_ALL_TESTS();
  // g++ tests/core-ob.tests.cpp -lgtest -lgtest_main -pthread -o
  // test_executable
}
