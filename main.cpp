#include <iostream>

#include "Order.h"
#include "OrderBook.h"
#include "utils/enums/OrderTypes.h"
#include "utils/enums/Side.h"

int main() {
  OrderBook orderBook = OrderBook();

  Order o1 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 100, 20);
  orderBook.AddOrder(o1); // ADDED

  Order o2 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 90, 5);
  orderBook.AddOrder(o2); // ADDED (DIFF LEVEL)

  Order o21 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 100, 77);
  orderBook.AddOrder(o21); // MULTIPLE ADDED (SAME LEVEL)
  orderBook.AddOrder(o2);  // NOT ADDED (REPEAT ORDER)

  std::cout << "BID" << std::endl;
  for (auto &ele : orderBook.getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev->getPrice() << " " << lev->getQuantity() << " "
              << lev->getOrderList().size() << std::endl;
  }

  std::cout << "----------------------" << std::endl;
  Order o3 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Sell, 95, 10);
  orderBook.AddOrder(o3);

  orderBook.CancelOrder(o1); // CANCELLED
  orderBook.CancelOrder(o1); // NOT CANCELLED (REPEAT)

  Order o4 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 93, 100);
  orderBook.ModifyOrder(o2, o4); // MODIFIED

  Order o5 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 107, 12);
  orderBook.AddOrder(o5);

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

  std::optional<Trade> MatchedTrade = orderBook.MatchPotentialOrders();

  if (!MatchedTrade.has_value()) {
    std::cout << "No fucking Matches at the moment" << std::endl;
    return 0;
  }
  std::cout << MatchedTrade->getMatchTime() << " "
            << MatchedTrade->getMatchedBid().price << " "
            << MatchedTrade->getMatchedBid().quantityFilled << " "
            << MatchedTrade->getMatchedAsk().price << " "
            << MatchedTrade->getMatchedAsk().quantityFilled << std::endl;

  std::cout << "----------------------" << std::endl;
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

  return 0;
}
