#include "include/OrderTraded.hpp"
#include "include/Trade.hpp"
#include "include/Xchange.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/ParticipantRel.hpp"
#include "utils/enums/Side.hpp"
#include <gtest/gtest.h>
#include <vector>

TEST(Participant, SetUpCheck)
{
  Xchange &xchange = Xchange::getInstance(3, 1000);
  ParticipantID partId = xchange.addParticipant("NUBE");
  const ParticipantPointer part = xchange.getParticipantInfo(partId);
  ASSERT_EQ(part->getParticipantID(), partId);
  ASSERT_EQ(part->getNumberOfAssetsInPortfolio(), 0);
  ASSERT_EQ(part->getNumberOfOrdersPlaced(), 0);
  ASSERT_EQ(part->getNumberOfTrades(), 0);
  Xchange::destroyInstance();
}

void verifyAddOrderInformation(const ParticipantPointer &partPtr,
                               const OrderID &orderId, const Symbol &symbol,
                               const Side::Side &side,
                               const OrderType::OrderType &otype,
                               const double &price, const Quantity &qty,
                               const std::string &activationTime,
                               const std::string &deactivationTime);

TEST(Participant, AddOrderRecorded)
{
  Xchange &xchange = Xchange::getInstance(3, 1000);
  ParticipantID partId = xchange.addParticipant("NUME");

  const std::string symbol = "SPY";
  xchange.tradeNewSymbol(symbol);
  std::optional<OrderID> orderId = xchange.placeOrder(
      partId, Actions::Actions::Add, std::nullopt, symbol, Side::Side::Buy,
      OrderType::OrderType::Market, 124.32, 4, "01-07-2025 19:12:27", "EOT");
  ASSERT_EQ(orderId.has_value(), true);
  const ParticipantPointer part = xchange.getParticipantInfo(partId);
  ASSERT_EQ(part->getNumberOfOrdersPlaced(), 1);
  ASSERT_EQ(part->isParticularOrderPlacedByParticipant(orderId.value()), true);

  verifyAddOrderInformation(xchange.getParticipantInfo(partId), orderId.value(),
                            symbol, Side::Side::Buy,
                            OrderType::OrderType::Market, 124.32, 4,
                            "01-07-2025 19:12:27", "EOT");
  Xchange::destroyInstance();
}

TEST(Participant, CancelOrderCleared)
{
  Xchange &xchange = Xchange::getInstance(30, 100000000);
  ParticipantID partId = xchange.addParticipant("NUME");
  const std::string symbol1 = "SPY";
  xchange.tradeNewSymbol(symbol1);
  std::optional<OrderID> orderId1 = xchange.placeOrder(
      partId, Actions::Actions::Add, std::nullopt, symbol1, Side::Side::Sell,
      OrderType::OrderType::AllOrNone, 4.97, 43, "09-07-2025 19:12:27", "EOT");

  std::optional<OrderID> orderId2 = xchange.placeOrder(
      partId, Actions::Actions::Add, std::nullopt, symbol1, Side::Side::Buy,
      OrderType::OrderType::Market, 124.32, 4, "01-07-2025 19:12:27", "EOT");

  xchange.placeOrder(partId, Actions::Actions::Cancel, orderId1.value(),
                     symbol1, Side::Side::Sell, OrderType::OrderType::AllOrNone,
                     std::nullopt, std::nullopt, std::nullopt, std::nullopt);

  verifyAddOrderInformation(xchange.getParticipantInfo(partId),
                            orderId2.value(), symbol1, Side::Side::Buy,
                            OrderType::OrderType::Market, 124.32, 4,
                            "01-07-2025 19:12:27", "EOT");

  ASSERT_NE(xchange.getParticipantInfo(partId), nullptr);
  ASSERT_EQ(
      xchange.getParticipantInfo(partId)->isParticularOrderPlacedByParticipant(
          orderId1.value()),
      false);
  Xchange::destroyInstance();
}

TEST(Participant, ModifyOrderRecorded)
{
  Xchange &xchange = Xchange::getInstance(30, 1000000);
  ParticipantID partId1 = xchange.addParticipant("ID1");
  const std::string symbol1 = "SPY";
  xchange.tradeNewSymbol(symbol1);
  std::optional<OrderID> orderId1 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, symbol1, Side::Side::Buy,
      OrderType::OrderType::Market, 124.32, 4, "01-07-2025 19:12:27", "EOT");

  verifyAddOrderInformation(xchange.getParticipantInfo(partId1),
                            orderId1.value(), symbol1, Side::Side::Buy,
                            OrderType::OrderType::Market, 124.32, 4,
                            "01-07-2025 19:12:27", "EOT");

  std::optional<OrderID> orderId2 =
      xchange.placeOrder(partId1, Actions::Actions::Modify, orderId1.value(),
                         symbol1, Side::Side::Buy, OrderType::OrderType::Market,
                         4.97, 43, "09-07-2025 19:12:27", "EOT");

  ASSERT_EQ(
      xchange.getParticipantInfo(partId1)->isParticularOrderPlacedByParticipant(
          orderId2.value()),
      true);
  ASSERT_EQ(
      xchange.getParticipantInfo(partId1)->isParticularOrderPlacedByParticipant(
          orderId1.value()),
      false);

  verifyAddOrderInformation(xchange.getParticipantInfo(partId1),
                            orderId2.value(), symbol1, Side::Side::Buy,
                            OrderType::OrderType::Market, 4.97, 43,
                            "09-07-2025 19:12:27", "EOT");
  Xchange::destroyInstance();
}

TEST(Participant, RecordExecutedTrades)
{
  Xchange &xchange = Xchange::getInstance(3, 100);
  ParticipantID partId1 = xchange.addParticipant("ID1");
  // mock functionality of xchange returning trades
  std::vector<Trade> recentTrades = {
      Trade(OrderTraded("SPY", 131221, 12.32, 4, partId1),
            OrderTraded("SPY", 842393, 12.32, 4, "20_INIF")),
      Trade(OrderTraded("APL", 1322421221, 4.97, 14, "23_NINIEV"),
            OrderTraded("APL", 842393122442424, 4.97, 14, partId1)),
      Trade(OrderTraded("PXL", 132242121221221, 15.68, 33, "2331_NINIEV"),
            OrderTraded("PXL", 842393121212424, 15.68, 33, "13_IDA"))};
  const ParticipantPointer partPtr = xchange.getParticipantInfo(partId1);
  partPtr->lastProcessedTradeIndex = 0;

  // comment out in recordTrades while mocking
  // if (m_orderComposition.count(matchedID) == 0)
  //   return;
  // Participant::updateOrderStatus(matchedID);

  partPtr->recordTrades(recentTrades);

  ASSERT_EQ(partPtr->getNumberOfTrades(), 2);
  ASSERT_EQ(partPtr->getNumberOfAssetsInPortfolio(), 2);
  ASSERT_EQ(partPtr->isSymbolInPortfolio("SPY"), true);
  ASSERT_EQ(partPtr->isSymbolInPortfolio("PXL"), false);
  ASSERT_EQ(partPtr->isSymbolInPortfolio("APL"), true);

  ASSERT_EQ(partPtr->getValuationOfSymbol("SPY"), 4 * 12.32);
  ASSERT_EQ(partPtr->getValuationOfSymbol("PXL"), 0);
  ASSERT_EQ(partPtr->getValuationOfSymbol("APL"), -14 * 4.97);
  ASSERT_NEAR(partPtr->getValuationOfPortfolio(), 4 * 12.32 - 14 * 4.97, 0.01);
  ASSERT_EQ(partPtr->lastProcessedTradeIndex, partPtr->getNumberOfTrades());
  Xchange::destroyInstance();
}

void verifyAddOrderInformation(const ParticipantPointer &partPtr,
                               const OrderID &orderId, const Symbol &symbol,
                               const Side::Side &side,
                               const OrderType::OrderType &otype,
                               const double &price, const Quantity &qty,
                               const std::string &activationTime,
                               const std::string &deactivationTime)
{

  const OrderPointer orderptr = partPtr->getOrder(orderId);
  ASSERT_NE(orderptr, nullptr);
  ASSERT_EQ(orderptr->getSymbol(), symbol);
  ASSERT_EQ(orderptr->getOrderType(), otype);
  ASSERT_EQ(orderptr->getParticipantID(), partPtr->getParticipantID());
  ASSERT_EQ(orderptr->getOrderID(), orderId);
  ASSERT_EQ(orderptr->getSide(), side);
  ASSERT_EQ(static_cast<double>(orderptr->getPrice()) / 100, price);
  ASSERT_EQ(orderptr->getRemainingQuantity(), qty);

  if (!activationTime.empty() && activationTime != "NOW")
  {
    ASSERT_EQ(partPtr->printParticipantLocalTime(orderptr->getActivationTime()), activationTime);
  }
  if (!deactivationTime.empty() && deactivationTime != "EOT")
  {
    ASSERT_EQ(partPtr->printParticipantLocalTime(orderptr->getDeactivationTime()), deactivationTime);
  }
  else
  {
    ASSERT_EQ(Order::returnReadableTime(orderptr->getDeactivationTime()),
              "01-01-2100 00:00:00");
  }
  // ASSERT_EQ(Order::returnReadableTime(orderptr->getActivationTime()),
  //           activationTime);
}
int main()
{
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
