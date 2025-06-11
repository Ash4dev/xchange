#include <iostream>
#include <optional>
#include <ostream>

#include "include/Order.hpp"
#include "include/OrderBook.hpp"
#include "include/Preprocess.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

void printMatchedTrades(std::vector<std::optional<Trade>> &trades) {
  for (auto &MatchedTrade : trades) {
    if (!MatchedTrade.has_value()) {
      std::cout << "No Matches at the moment" << std::endl;
      std::cout << "|||||||||||||||||||" << std::endl;
      continue;
    }
    MatchedTrade->printMatchTime();
    std::cout << MatchedTrade->getMatchedBid().price << " "
              << MatchedTrade->getMatchedBid().quantityFilled << " "
              << MatchedTrade->getMatchedBid().orderID << " "
              << MatchedTrade->getMatchedAsk().orderID << std::endl;
    std::cout << "|||||||||||||||||||" << std::endl;
  }
}

int main() {

  std::string symbol = "A";

  // create separate processor for buy and sell
  // sorting condition different
  PreProcessor bidpreProcessor = PreProcessor(symbol);
  PreProcessor askpreProcessor = PreProcessor(symbol);
  OrderBook orderBook = OrderBook(symbol);

  // PreProcessor && orderBook must stay connected somehow else
  // not able to do insert and stuff

  Order o1 = Order(symbol, OrderType::OrderType::GoodTillCancel,
                   Side::Side::Buy, 90.39, 20);
  bidpreProcessor.AddOrderInOrderBook(o1);
  Order o2 = Order(symbol, OrderType::OrderType::GoodTillCancel,
                   Side::Side::Sell, 93.04, 5);
  askpreProcessor.AddOrderInOrderBook(o2);
  Order o3 = Order(symbol, OrderType::OrderType::GoodAfterTime, Side::Side::Buy,
                   96.15, 77, "04-06-2025 13:04:26");
  bidpreProcessor.AddOrderInOrderBook(o3);

  Order o4 = Order("A", OrderType::OrderType::MarketOnOpen, Side::Side::Sell,
                   99.86, 100);
  askpreProcessor.AddOrderInOrderBook(o4);
  Order o5 =
      Order("A", OrderType::OrderType::FillOrKill, Side::Side::Sell, 92.43, 43);
  askpreProcessor.AddOrderInOrderBook(o5);
  askpreProcessor.CancelOrderFromOrderBook(o4.getOrderID());
  Order o6 = Order("A", OrderType::OrderType::ImmediateOrCancel,
                   Side::Side::Buy, 97.37, 21);
  bidpreProcessor.AddOrderInOrderBook(o6);
  bidpreProcessor.CancelOrderFromOrderBook(o4.getOrderID());

  std::cout << "BIDS" << std::endl;
  bidpreProcessor.printPreProcessorStatus();
  std::cout << std::endl;
  std::cout << "ASKS" << std::endl;
  askpreProcessor.printPreProcessorStatus();
  std::cout << std::endl;
  return 0;
}
