#include <iostream>

#include "Order.h"
#include "OrderBook.h"
#include "utils/enums/OrderTypes.h"
#include "utils/enums/Side.h"

int main() {
  OrderBook orderBook = OrderBook("A");
  std::vector<std::optional<Trade>> trades;
  Order o1 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy,
                   100.32, 20);
  trades.push_back(orderBook.AddOrder(o1)); // ADDED

  Order o2 =
      Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 90, 5);
  trades.push_back(orderBook.AddOrder(o2)); // ADDED (DIFF LEVEL)

  Order o21 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy,
                    100, 77);
  trades.push_back(orderBook.AddOrder(o21)); // MULTIPLE ADDED (SAME LEVEL)
  trades.push_back(orderBook.AddOrder(o2));  // NOT ADDED (REPEAT ORDER)

  std::cout << "BID" << std::endl;
  for (auto &ele : orderBook.getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev->getPrice() << " " << lev->getQuantity() << " "
              << lev->getOrderList().size() << std::endl;
  }
  std::cout << "----------------------" << std::endl;

  Order o3 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Sell,
                   95, 10);
  trades.push_back(orderBook.AddOrder(o3));
  Order o31 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Sell,
                    195, 23);
  trades.push_back(orderBook.AddOrder(o31));

  // o1 matched with o21 (o2 leads to segmentation fault check once)
  trades.push_back(orderBook.CancelOrder(o21.getOrderID())); // CANCELLED
  trades.push_back(
      orderBook.CancelOrder(o21.getOrderID())); // NOT CANCELLED (REPEAT)

  Order o4 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy,
                   93, 100);
  trades.push_back(orderBook.ModifyOrder(o2.getOrderID(), o4)); // MODIFIED

  Order o5 = Order("A", OrderType::OrderType::GoodTillCancel, Side::Side::Buy,
                   107, 12);
  trades.push_back(orderBook.AddOrder(o5));

  std::cout << "BID" << std::endl;
  for (auto &ele : orderBook.getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev->getPrice() << " " << lev->getQuantity() << " "
              << lev->getOrderList().size() << std::endl;
  }
  std::cout << "----------------------" << std::endl;
  std::cout << "ASK" << std::endl;
  for (auto &ele : orderBook.getAskLevels()) {
    auto &lev = ele.second;
    std::cout << lev->getPrice() << " " << lev->getQuantity() << " "
              << lev->getOrderList().size() << std::endl;
  }
  std::cout << "----------------------" << std::endl;

  for (auto &MatchedTrade : trades) {
    if (!MatchedTrade.has_value()) {
      std::cout << "No Matches at the moment" << std::endl;
      std::cout << "|||||||||||||||||||" << std::endl;
      continue;
    }
    std::cout << MatchedTrade->getMatchTime() << " "
              << MatchedTrade->getMatchedBid().price << " "
              << MatchedTrade->getMatchedBid().quantityFilled << " "
              << MatchedTrade->getMatchedBid().orderID << " "
              << MatchedTrade->getMatchedAsk().orderID << std::endl;
    std::cout << "|||||||||||||||||||" << std::endl;
  }
  return 0;
}
