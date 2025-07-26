#include "include/Xchange.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/PreProcessorRel.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <chrono>
#include <cstddef>
#include <gtest/gtest.h>
#include <unistd.h>

TEST(PreProcessor, SetUpCheck)
{
    Xchange &xchange = Xchange::getInstance(3, 1000, "Europe/London");
    xchange.tradeNewSymbol("SPY");
    // tradeNewSymbol check performed by Xchange test suite

    const PreProcessorPointer preBid =
        xchange.getPreProcessor("SPY", Side::Side::Buy);
    ASSERT_EQ(preBid->getMaxPendingOrdersThreshold(), 3);
    ASSERT_EQ(preBid->getMaxPendingDuration().count(), 1000);
    ASSERT_EQ(preBid->getTimeZone(), "Europe/London");

    for (std::size_t iter = 0; iter < preBid->getNumberOfOrderTypes(); iter++)
    {
        ASSERT_EQ(preBid->NumberOfOrdersBeingProcessed(
                      static_cast<OrderType::OrderType>(iter)),
                  0);
    }

    const PreProcessorPointer preAsk =
        xchange.getPreProcessor("SPY", Side::Side::Sell);
    ASSERT_EQ(preAsk->getMaxPendingOrdersThreshold(), 3);
    ASSERT_EQ(preAsk->getMaxPendingDuration().count(), 1000);
    ASSERT_EQ(preAsk->getTimeZone(), "Europe/London");

    for (std::size_t iter = 0; iter < preAsk->getNumberOfOrderTypes(); iter++)
    {
        ASSERT_EQ(preAsk->NumberOfOrdersBeingProcessed(
                      static_cast<OrderType::OrderType>(iter)),
                  0);
    }
    Xchange::destroyInstance();
}

// Insertion into / Removal from PreProcessor verified during Xchange testing
// for Order Addition, Cancellation and Modification

TEST(PreProcessor, IsFlushedDueToOrderThresold)
{
    Xchange &xchange = Xchange::getInstance(3, 1000000);
    xchange.tradeNewSymbol("SPY");
    xchange.tradeNewSymbol("APL");
    ParticipantID partId1 = xchange.addParticipant("LOLOLO");
    ParticipantID partId2 = xchange.addParticipant("YOLLOOO");

    std::optional<OrderID> orderId1 =
        xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::GoodTillCancel,
                           124.32, 4, "21-07-2025 16:43:57", "EOT");
    ASSERT_EQ(
        xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
        1);
    std::optional<OrderID> orderId2 =
        xchange.placeOrder(partId2, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::GoodAfterTime,
                           121.89, 1, "20-07-2025 19:12:27", "EOT");
    ASSERT_EQ(
        xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
        2);
    std::optional<OrderID> orderId3 =
        xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, "APL",
                           Side::Side::Buy, OrderType::OrderType::FillOrKill,
                           1243.2, 23, "21-07-2025 16:43:57", "EOT");
    ASSERT_EQ(
        xchange.getPreProcessor("APL", Side::Side::Buy)->getBufferedOrderCount(),
        1);
    ASSERT_EQ(
        xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
        2);
    std::optional<OrderID> orderId4 =
        xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::AllOrNone,
                           119.90, 67, "01-08-2025 19:12:27", "EOT");
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

TEST(PreProcessor, IsFlushedDueToDurationThresold)
{
    Xchange &xchange = Xchange::getInstance(30, 1);
    xchange.tradeNewSymbol("SPY");
    xchange.tradeNewSymbol("APL");
    ParticipantID partId1 = xchange.addParticipant("LOLOLO");
    ParticipantID partId2 = xchange.addParticipant("YOLLOOO");

    std::optional<OrderID> orderId1 =
        xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::GoodTillCancel,
                           124.32, 4, "21-07-2025 16:43:57", "EOT");
    ASSERT_EQ(
        xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
        1);
    std::optional<OrderID> orderId2 =
        xchange.placeOrder(partId2, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::GoodAfterTime,
                           121.89, 1, "20-07-2025 19:12:27", "EOT");
    ASSERT_EQ(
        xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
        2);
    std::optional<OrderID> orderId3 =
        xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, "APL",
                           Side::Side::Buy, OrderType::OrderType::FillOrKill,
                           1243.2, 23, "21-07-2025 16:43:57", "EOT");
    sleep(1);
    ASSERT_EQ(
        xchange.getPreProcessor("APL", Side::Side::Buy)->getBufferedOrderCount(),
        1);
    ASSERT_EQ(
        xchange.getPreProcessor("SPY", Side::Side::Buy)->getBufferedOrderCount(),
        2);
    std::optional<OrderID> orderId4 =
        xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::AllOrNone,
                           119.90, 67, "01-08-2025 19:12:27", "EOT");
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

//
TEST(PreProcessor, MarketOnCloseAttributesUpdated)
{
    Xchange &xchange = Xchange::getInstance(9, 1000);
    const std::string symbol = "SPY";
    xchange.tradeNewSymbol(symbol);
    ParticipantID partId = xchange.addParticipant("NUB");

    std::optional<OrderID> orderId =
        xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::MarketOnClose,
                           124.32, 4, "21-07-2025 16:43:57", "EOT");

    const OrderPointer &orderptr =
        xchange.getPreProcessor(symbol, Side::Side::Buy)
            ->getOrder(orderId.value());
    EXPECT_EQ(xchange.getParticipantInfo(partId)->printParticipantLocalTime(orderptr->getActivationTime()),
              "28-07-2025 15:25:00");
    EXPECT_EQ(xchange.getParticipantInfo(partId)->printParticipantLocalTime(orderptr->getDeactivationTime()),
              "28-07-2025 15:30:00");
    Xchange::destroyInstance();
}

TEST(PreProcessor, MarketOnOpenAttributesUpdated)
{
    Xchange &xchange = Xchange::getInstance(9, 1000);
    const std::string symbol = "SPY";
    xchange.tradeNewSymbol(symbol);
    ParticipantID partId = xchange.addParticipant("NUB");

    std::optional<OrderID> orderId =
        xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
                           Side::Side::Buy, OrderType::OrderType::MarketOnOpen,
                           124.32, 4, "21-07-2025 16:43:57", "EOT");

    const OrderPointer &orderptr =
        xchange.getPreProcessor(symbol, Side::Side::Buy)
            ->getOrder(orderId.value());
    EXPECT_EQ(xchange.getParticipantInfo(partId)->printParticipantLocalTime(orderptr->getActivationTime()),
              "28-07-2025 09:15:00");
    EXPECT_EQ(xchange.getParticipantInfo(partId)->printParticipantLocalTime(orderptr->getDeactivationTime()),
              "28-07-2025 09:20:00");
    Xchange::destroyInstance();
}

// //
// TEST(PreProcessor, AreOrdersRanked) {
//   Xchange &xchange = Xchange::getInstance(9, 1000);
//   const std::string symbol = "SPY";
//   xchange.tradeNewSymbol(symbol);
//   ParticipantID partId = xchange.addParticipant("NUB");
//   std::optional<OrderID> orderId1 = xchange.placeOrder(
//       partId, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
//       OrderType::OrderType::Market, 124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId2 =
//       xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
//                          Side::Side::Buy, OrderType::OrderType::MarketOnClose,
//                          124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId3 =
//       xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
//                          Side::Side::Buy, OrderType::OrderType::MarketOnOpen,
//                          124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId4 =
//       xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
//                          Side::Side::Buy, OrderType::OrderType::FillOrKill,
//                          124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId5 = xchange.placeOrder(
//       partId, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
//       OrderType::OrderType::ImmediateOrCancel, 124.32, 4, "21-07-2025 16:43:57",
//       "EOT");

//   std::optional<OrderID> orderId6 =
//       xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
//                          Side::Side::Buy, OrderType::OrderType::GoodAfterTime,
//                          124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId7 =
//       xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
//                          Side::Side::Buy, OrderType::OrderType::GoodForDay,
//                          124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId8 =
//       xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
//                          Side::Side::Buy, OrderType::OrderType::GoodTillDate,
//                          124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId9 = xchange.placeOrder(
//       partId, Actions::Actions::Add, std::nullopt, "SPY", Side::Side::Buy,
//       OrderType::OrderType::AllOrNone, 124.32, 4, "21-07-2025 16:43:57", "EOT");

//   std::optional<OrderID> orderId10 =
//       xchange.placeOrder(partId, Actions::Actions::Add, std::nullopt, "SPY",
//                          Side::Side::Buy, OrderType::OrderType::GoodTillCancel,
//                          124.32, 4, "21-07-2025 16:43:57", "EOT");

//   Xchange::destroyInstance();
// }
//
// TEST(PreProcessor, QueueOrderCheck) { ; }
//
// TEST(PreProcessor, EmptyTypeRankedOrdersOrderCheck) { ; }
//
// TEST(PreProcessor, EmptyWaitQueueCheck) { ; }
//
// TEST(PreProcessor, IsOrderClearedPostMatch) { ; }

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
