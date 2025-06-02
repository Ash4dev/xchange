#include <iostream>
#include <optional>

#include "Order.h"
#include "OrderBook.h"
#include "utils/enums/OrderTypes.h"
#include "utils/enums/Side.h"

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
  OrderBook orderBook = OrderBook("A");
  std::vector<std::optional<Trade>> trades;
  Order o1 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy,
                   90.39, 20);
  trades.push_back(orderBook.AddOrder(o1)); // ADDED

  Order o2 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy,
                   93.04, 5);
  trades.push_back(orderBook.AddOrder(o2)); // ADDED (DIFF LEVEL)

  Order o21 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy,
                    96.15, 77);
  trades.push_back(orderBook.AddOrder(o21)); // MULTIPLE ADDED (SAME LEVEL)
  trades.push_back(orderBook.AddOrder(o2));  // NOT ADDED (REPEAT ORDER)

  orderBook.printOrderBookState("o1-o21 added");

  Order o3 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Sell,
                   99.86, 100);
  trades.push_back(orderBook.AddOrder(o3));
  Order o31 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Sell,
                    92.43, 43);
  trades.push_back(orderBook.AddOrder(o31));
  Order ox = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Sell,
                   97.37, 21);
  trades.push_back(orderBook.AddOrder(ox));

  orderBook.printOrderBookState("o3-ox added");

  trades.push_back(orderBook.CancelOrder(o3.getOrderID())); // CANCELLED
  trades.push_back(
      orderBook.CancelOrder(o3.getOrderID())); // NOT CANCELLED (REPEAT)

  orderBook.printOrderBookState("o3 cancelled");

  Order o4 =
      Order("A", OrderType::OrderType::Market, Side::Side::Buy, 93.83, 100);
  trades.push_back(orderBook.ModifyOrder(o31.getOrderID(), o4)); // MODIFIED

  Order o5 =
      Order("A", OrderType::OrderType::Market, Side::Side::Sell, 92.45, 12);
  trades.push_back(orderBook.AddOrder(o5));

  orderBook.printOrderBookState("o2 mod to o4, o5 added");

  printMatchedTrades(trades);

  return 0;
}
