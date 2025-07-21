#include "include/Xchange.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/PreProcessorRel.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(PreProcessor, SetUpCheck) {
  Xchange &xchange = Xchange::getInstance(3, 1000);
  xchange.tradeNewSymbol("SPY");
  // tradeNewSymbol check performed by Xchange test suite

  const PreProcessorPointer preBid =
      xchange.getPreProcessor("SPY", Side::Side::Buy);
  ASSERT_EQ(preBid->getMaxPendingOrdersThreshold(), 3);
  ASSERT_EQ(preBid->getMaxPendingDuration().count(), 1000);

  for (std::size_t iter = 0; iter < preBid->getNumberOfOrderTypes(); iter++) {
    ASSERT_EQ(preBid->NumberOfOrdersBeingProcessed(
                  static_cast<OrderType::OrderType>(iter)),
              0);
  }

  const PreProcessorPointer preAsk =
      xchange.getPreProcessor("SPY", Side::Side::Sell);
  ASSERT_EQ(preBid->getMaxPendingOrdersThreshold(), 3);
  ASSERT_EQ(preBid->getMaxPendingDuration().count(), 1000);

  for (std::size_t iter = 0; iter < preAsk->getNumberOfOrderTypes(); iter++) {
    ASSERT_EQ(preAsk->NumberOfOrdersBeingProcessed(
                  static_cast<OrderType::OrderType>(iter)),
              0);
  }
  Xchange::destroyInstance();
}

// Insertion into / Removal from PreProcessor verified during Xchange testing
// for Order Addition, Cancellation and Modification

TEST(PreProcessor, IsFlushedDueToOrderThresold) {
  Xchange &xchange = Xchange::getInstance(3, 1000000);
  xchange.tradeNewSymbol("SPY");
  xchange.tradeNewSymbol("APL");
  ParticipantID partId1 = xchange.addParticipant("LOLOLO");
  ParticipantID partId2 = xchange.addParticipant("YOLLOOO");

  std::optional<OrderID> orderId1 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
      OrderType::OrderType::GoodTillCancel, 124.32, 4, "01-07-2025 19:12:27",
      "01-01-2100 00:00:00");
  ASSERT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      1);
  std::optional<OrderID> orderId2 = xchange.placeOrder(
      partId2, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
      OrderType::OrderType::GoodAfterTime, 121.89, 1, "20-07-2025 19:12:27",
      "01-01-2100 00:00:00");
  ASSERT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      2);
  std::optional<OrderID> orderId3 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, "APL", Side::Side::Buy,
      OrderType::OrderType::FillOrKill, 1243.2, 23, "01-07-2025 19:12:27",
      "01-01-2100 00:00:00");
  ASSERT_EQ(
      xchange.getPreProcessor("APL", Side::Side::Buy)->getBufferedOrderCount(),
      1);
  ASSERT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      2);
  std::optional<OrderID> orderId4 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
      OrderType::OrderType::AllOrNone, 119.90, 67, "01-08-2025 19:12:27",
      "01-01-2100 00:00:00");
  ASSERT_EQ(
      xchange.getPreProcessor("APL", Side::Side::Buy)->getBufferedOrderCount(),
      1);
  // flush started and GoodTillCancel & GoodAfterTime order can be inserted
  EXPECT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      3 - 2);
  (orderId1.has_value() || orderId2.has_value() || orderId3.has_value() ||
   orderId4.has_value());
  Xchange::destroyInstance();
}

TEST(PreProcessor, IsFlushedDueToDurationThresold) {
  Xchange &xchange = Xchange::getInstance(30, 1);
  xchange.tradeNewSymbol("SPY");
  xchange.tradeNewSymbol("APL");
  ParticipantID partId1 = xchange.addParticipant("LOLOLO");
  ParticipantID partId2 = xchange.addParticipant("YOLLOOO");

  std::optional<OrderID> orderId1 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
      OrderType::OrderType::GoodTillCancel, 124.32, 4, "01-07-2025 19:12:27",
      "01-01-2100 00:00:00");
  ASSERT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      1);
  std::optional<OrderID> orderId2 = xchange.placeOrder(
      partId2, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
      OrderType::OrderType::GoodAfterTime, 121.89, 1, "20-07-2025 19:12:27",
      "01-01-2100 00:00:00");
  ASSERT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      2);
  std::optional<OrderID> orderId3 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, "APL", Side::Side::Buy,
      OrderType::OrderType::FillOrKill, 1243.2, 23, "01-07-2025 19:12:27",
      "01-01-2100 00:00:00");
  for (int i = 0; i < 100000; i++) {
    for (int j = 0; j < 100000; j++) {
      ;
    }
  }
  ASSERT_EQ(
      xchange.getPreProcessor("APL", Side::Side::Buy)->getBufferedOrderCount(),
      1);
  ASSERT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      2);
  std::optional<OrderID> orderId4 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
      OrderType::OrderType::AllOrNone, 119.90, 67, "01-08-2025 19:12:27",
      "01-01-2100 00:00:00");
  ASSERT_EQ(
      xchange.getPreProcessor("APL", Side::Side::Buy)->getBufferedOrderCount(),
      1);
  // flush started and GoodTillCancel & GoodAfterTime order can be inserted
  EXPECT_EQ(
      xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
      3 - 2);
  (orderId1.has_value() || orderId2.has_value() || orderId3.has_value() ||
   orderId4.has_value());
  Xchange::destroyInstance();
}

// TEST(PreProcessor, WHatTheFuck) {
//   std::multiset<PreProcessor::OrderActionInfo> mst;
//   PreProcessor::OrderActionInfo o1 = PreProcessor::OrderActionInfo(
//       23792372838923, OrderType::OrderType::AllOrNone,
//       Actions::Actions::Add);
//   PreProcessor::OrderActionInfo o2 = PreProcessor::OrderActionInfo(
//       2378923, OrderType::OrderType::GoodAfterTime, Actions::Actions::Add);
//   PreProcessor::OrderActionInfo o3 = PreProcessor::OrderActionInfo(
//       237923723, OrderType::OrderType::Market, Actions::Actions::Add);
//   mst.insert(o1);
//   mst.insert(o2);
//   mst.insert(o3);
//
//   std::multiset<PreProcessor::OrderActionInfo> copy = mst;
//   for (auto ele : copy) {
//     if (mst.find(ele) == mst.end()) {
//       std::cout << "NOT FOUND" << std::endl;
//       continue;
//     }
//     std::cout << "FOUND" << std::endl;
//   }
// }

// TEST(PreProcessor, QueueOrderCheck) { ; }
//
// TEST(PreProcessor, EmptyTypeRankedOrdersOrderCheck) { ; }
//
// TEST(PreProcessor, EmptyWaitQueueCheck) { ; }
//
// TEST(PreProcessor, IsOrderClearedPostMatch) { ; }

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
