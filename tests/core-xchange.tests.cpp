#include "include/Preprocess.hpp"
#include "include/Xchange.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/alias/ParticipantRel.hpp"
#include "utils/alias/PreProcessorRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"
#include <gtest/gtest.h>
#include <optional>
#include <string>

TEST(Xchage, SetUpCheck) {
  Xchange &xchange = Xchange::getInstance(3, 1000);
  ASSERT_EQ(xchange.getOrderThreshold(), 3);
  ASSERT_EQ(xchange.getDurationThreshold(), 1000);
  ASSERT_EQ(xchange.getParticipantCount(), 0);
  ASSERT_EQ(xchange.getSymbolsTradedCount(), 0);
  Xchange::destroyInstance();
}

TEST(Xchange, AddParticipant) {
  Xchange &xchange = Xchange::getInstance(3, 1000);

  std::string govID = "HIHD2_J";
  ParticipantID partId = xchange.addParticipant(govID);

  ASSERT_EQ(static_cast<int>(xchange.getParticipantCount()), 1);
  ASSERT_EQ(xchange.generateParticipantID(govID), "0_" + govID);
  ASSERT_EQ(xchange.canMapGovIDToParticipantID(govID), true);
  ASSERT_EQ(xchange.isGovIDPresent(govID), true);
  ASSERT_EQ(xchange.isParticipantIDPresent(partId), true);

  const ParticipantPointer corrPart = xchange.getParticipantInfo(partId);
  ASSERT_NE(corrPart, nullptr);
  ASSERT_EQ(corrPart->getParticipantID(), partId);
  ASSERT_EQ(corrPart->getNumberOfAssetsInPortfolio(), 0);
  ASSERT_EQ(corrPart->getNumberOfOrdersPlaced(), 0);
  ASSERT_EQ(corrPart->getNumberOfTrades(), 0);

  Xchange::destroyInstance();
}

TEST(Xchange, RemoveParticipant) {
  Xchange &xchange = Xchange::getInstance(3, 1000);
  const std::string govId1 = "NUB3";
  const std::string govId2 = "NuB9";
  ParticipantID partId1 = xchange.addParticipant(govId1);
  ParticipantID partId2 = xchange.addParticipant(govId2);
  ASSERT_EQ(static_cast<int>(xchange.getParticipantCount()), 2);
  xchange.removeParticipant(partId1);
  ASSERT_EQ(static_cast<int>(xchange.getParticipantCount()), 1);
  ASSERT_EQ(xchange.canMapGovIDToParticipantID(govId1), false);
  ASSERT_EQ(xchange.isGovIDPresent(govId1), false);
  ASSERT_EQ(xchange.isParticipantIDPresent(partId1), false);
  Xchange::destroyInstance();
}

TEST(Xchange, TradeNewSymbol) {
  Xchange &xchange = Xchange::getInstance(3, 1000);
  ASSERT_EQ(xchange.getSymbolsTradedCount(), 0);
  const std::string symbol = "SPY";
  xchange.tradeNewSymbol(symbol);
  ASSERT_EQ(xchange.getSymbolsTradedCount(), 1);
  ASSERT_EQ(xchange.isSymbolTraded(symbol), true);
  // corresponding orderbook, preprocessors created for the symbol
  ASSERT_NE(xchange.getOrderBook(symbol), nullptr);
  ASSERT_NE(xchange.getPreProcessor(symbol, Side::Side::Buy), nullptr);
  ASSERT_NE(xchange.getPreProcessor(symbol, Side::Side::Sell), nullptr);
  Xchange::destroyInstance();
}

TEST(Xchange, RetireOldSymbol) {
  Xchange &xchange = Xchange::getInstance(3, 1000);
  const std::string symbol1 = "SPY";
  const std::string symbol2 = "APL";
  xchange.tradeNewSymbol(symbol1);
  xchange.tradeNewSymbol(symbol2);
  ASSERT_EQ(xchange.getSymbolsTradedCount(), 2);
  ASSERT_EQ(xchange.isSymbolTraded(symbol1), true);
  ASSERT_EQ(xchange.isSymbolTraded(symbol2), true);
  xchange.retireOldSymbol(symbol1);
  ASSERT_EQ(xchange.getSymbolsTradedCount(), 1);
  ASSERT_EQ(xchange.isSymbolTraded(symbol1), false);
  ASSERT_EQ(xchange.isSymbolTraded(symbol2), true);
  Xchange::destroyInstance();
}

void verifyAddOrderInformation(
    const ParticipantID &partId, const PreProcessorPointer &relPre,
    const Actions::Actions &action, const OrderID &orderId,
    const Symbol &symbol, const Side::Side &side,
    const OrderType::OrderType &otype, const double &price, const Quantity &qty,
    const std::string &activationTime, const std::string &deactivationTime);

TEST(Xchange, PlaceAddOrder) {
  // ensure that order stays in preprocessor
  Xchange &xchange = Xchange::getInstance(30, 1000000);
  ParticipantID partId1 = xchange.addParticipant("ID1");
  ParticipantID partId2 = xchange.addParticipant("ID3");
  const std::string symbol1 = "SPY";
  xchange.tradeNewSymbol(symbol1);
  std::optional<OrderID> orderId1 =
      xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, symbol1,
                         Side::Side::Buy, OrderType::OrderType::Market, 124.32,
                         4, "01-07-2025 19:12:27", "01-01-2100 00:00:00");

  ASSERT_EQ(orderId1.has_value(), true);
  const PreProcessorPointer relPre =
      xchange.getPreProcessor(symbol1, Side::Side::Buy);

  verifyAddOrderInformation(partId1, relPre, Actions::Actions::Add,
                            orderId1.value(), symbol1, Side::Side::Buy,
                            OrderType::OrderType::Market, 124.32, 4,
                            "01-07-2025 19:12:27", "01-01-2100 00:00:00");
  Xchange::destroyInstance();
}

void verifyCancelOrderInformation(const PreProcessorPointer &relPre,
                                  const OrderID &orderId);

TEST(Xchange, PlaceCancelOrder) {
  Xchange &xchange = Xchange::getInstance(30, 1000000);
  ParticipantID partId1 = xchange.addParticipant("ID1");
  const std::string symbol1 = "SPY";
  const std::string symbol2 = "APL";
  xchange.tradeNewSymbol(symbol1);
  std::optional<OrderID> orderId1 =
      xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, symbol1,
                         Side::Side::Buy, OrderType::OrderType::Market, 124.32,
                         4, "01-07-2025 19:12:27", "01-01-2100 00:00:00");

  std::optional<OrderID> orderId2 = xchange.placeOrder(
      partId1, Actions::Actions::Add, std::nullopt, symbol1, Side::Side::Sell,
      OrderType::OrderType::AllOrNone, 4.97, 43, "09-07-2025 19:12:27",
      "01-01-2100 00:00:00");

  verifyAddOrderInformation(
      partId1, xchange.getPreProcessor(symbol1, Side::Side::Buy),
      Actions::Actions::Add, orderId1.value(), symbol1, Side::Side::Buy,
      OrderType::OrderType::Market, 124.32, 4, "01-07-2025 19:12:27",
      "01-01-2100 00:00:00");
  verifyAddOrderInformation(
      partId1, xchange.getPreProcessor(symbol1, Side::Side::Sell),
      Actions::Actions::Add, orderId2.value(), symbol1, Side::Side::Sell,
      OrderType::OrderType::AllOrNone, 4.97, 43, "09-07-2025 19:12:27",
      "01-01-2100 00:00:00");

  xchange.placeOrder(partId1, Actions::Actions::Cancel, orderId1.value(),
                     symbol1, std::nullopt, OrderType::OrderType::Market,
                     std::nullopt, std::nullopt, std::nullopt, std::nullopt);

  verifyCancelOrderInformation(
      xchange.getPreProcessor(symbol1, Side::Side::Buy), orderId1.value());
  Xchange::destroyInstance();
}

TEST(Xchange, PlaceModifyOrder) {
  Xchange &xchange = Xchange::getInstance(30, 1000000);
  ParticipantID partId1 = xchange.addParticipant("ID1");
  const std::string symbol1 = "SPY";
  xchange.tradeNewSymbol(symbol1);
  std::optional<OrderID> orderId1 =
      xchange.placeOrder(partId1, Actions::Actions::Add, std::nullopt, symbol1,
                         Side::Side::Buy, OrderType::OrderType::Market, 124.32,
                         4, "01-07-2025 19:12:27", "01-01-2100 00:00:00");

  verifyAddOrderInformation(
      partId1, xchange.getPreProcessor(symbol1, Side::Side::Buy),
      Actions::Actions::Add, orderId1.value(), symbol1, Side::Side::Buy,
      OrderType::OrderType::Market, 124.32, 4, "01-07-2025 19:12:27",
      "01-01-2100 00:00:00");

  std::optional<OrderID> orderId2 = xchange.placeOrder(
      partId1, Actions::Actions::Modify, orderId1.value(), symbol1,
      Side::Side::Buy, OrderType::OrderType::Market, 4.97, 43,
      "09-07-2025 19:12:27", "01-01-2100 00:00:00");

  ASSERT_EQ(orderId2.has_value(), true);
  verifyAddOrderInformation(partId1,
                            xchange.getPreProcessor(symbol1, Side::Side::Buy),
                            Actions::Actions::Add, orderId2.value(), symbol1,
                            Side::Side::Buy, OrderType::OrderType::Market, 4.97,
                            43, "09-07-2025 19:12:27", "01-01-2100 00:00:00");

  Xchange::destroyInstance();
}

void verifyAddOrderInformation(
    const ParticipantID &partId, const PreProcessorPointer &relPre,
    const Actions::Actions &action, const OrderID &orderId,
    const Symbol &symbol, const Side::Side &side,
    const OrderType::OrderType &otype, const double &price, const Quantity &qty,
    const std::string &activationTime, const std::string &deactivationTime) {
  ASSERT_NE(relPre, nullptr);
  ASSERT_EQ(relPre->hasOrderBeenEncountered(orderId), true);
  std::optional<PreProcessor::OrderActionInfo> ordactinfo =
      relPre->getOrderInfo(orderId);
  ASSERT_EQ(ordactinfo.has_value(), true);
  auto const &[_, type, actionStored] = ordactinfo.value();
  ASSERT_EQ(_, orderId);
  ASSERT_EQ(type, otype);
  ASSERT_EQ(actionStored, action);

  const OrderPointer orderptr = relPre->getOrder(orderId);
  ASSERT_NE(orderptr, nullptr);
  ASSERT_EQ(orderptr->getSymbol(), symbol);
  ASSERT_EQ(orderptr->getParticipantID(), partId);
  ASSERT_EQ(orderptr->getOrderID(), orderId);
  ASSERT_EQ(orderptr->getSide(), side);
  ASSERT_EQ(static_cast<double>(orderptr->getPrice()) / 100, price);
  ASSERT_EQ(orderptr->getRemainingQuantity(), qty);
  ASSERT_EQ(Order::returnReadableTime(orderptr->getActivationTime()),
            activationTime);
  ASSERT_EQ(Order::returnReadableTime(orderptr->getDeactivationTime()),
            deactivationTime);
}

void verifyCancelOrderInformation(const PreProcessorPointer &relPre,
                                  const OrderID &orderId) {
  ASSERT_NE(relPre, nullptr);
  ASSERT_EQ(relPre->hasOrderBeenEncountered(orderId), true);
  std::optional<PreProcessor::OrderActionInfo> ordactinfo =
      relPre->getOrderInfo(orderId);
  ASSERT_EQ(ordactinfo.has_value(), false);

  const OrderPointer orderptr = relPre->getOrder(orderId);
  ASSERT_EQ(orderptr, nullptr);
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
