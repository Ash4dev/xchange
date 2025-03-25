#include <iostream>

#include "Order.h"
#include "OrderBook.h"
#include "utils/enums/OrderTypes.h"
#include "utils/enums/Side.h"

int main() {
  OrderBook orderBook = OrderBook();

  Order o1 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 100, 20);
  orderBook.AddOrder(o1);

  for (auto &ele : orderBook.getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev.getPrice() << " " << lev.getQuantity()
              << lev.getOrderList().size() << std::endl;
  }

  std::cout << "----------------------" << std::endl;
  Order o2 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 90, 5);
  orderBook.AddOrder(o2);

  for (auto &ele : orderBook.getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev.getPrice() << " " << lev.getQuantity()
              << lev.getOrderList().size() << std::endl;
  }

  std::cout << "----------------------" << std::endl;
  Order o3 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Sell, 95, 10);
  orderBook.AddOrder(o3);

  orderBook.CancelOrder(o1);

  Order o4 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 93, 100);
  orderBook.ModifyOrder(o2, o4);

  Order o5 =
      Order(OrderType::OrderType::GoodTillCancel, Side::Side::Buy, 107, 12);
  orderBook.AddOrder(o5);

  for (auto &ele : orderBook.getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev.getPrice() << " " << lev.getQuantity()
              << lev.getOrderList().size() << std::endl;
  }

  std::cout << "----------------------" << std::endl;
  for (auto &ele : orderBook.getAskLevels()) {
    auto &lev = ele.second;
    std::cout << lev.getPrice() << " " << lev.getQuantity()
              << lev.getOrderList().size() << std::endl;
  }
  std::cout << "----------------------" << std::endl;

  std::optional<Trade> lol = orderBook.MatchPotentialOrders();

  if (!lol.has_value()) {
    std::cout << "No fucking Matches at the moment" << std::endl;
    return 0;
  }
  std::cout << lol->getMatchTime() << " " << lol->getMatchedBid().price << " "
            << lol->getMatchedBid().quantityFilled << " "
            << lol->getMatchedAsk().price << " "
            << lol->getMatchedAsk().quantityFilled << std::endl;

  std::cout << "----------------------" << std::endl;
  for (auto &ele : orderBook.getBidLevels()) {
    auto &lev = ele.second;
    std::cout << lev.getPrice() << " " << lev.getQuantity()
              << lev.getOrderList().size() << std::endl;
  }

  std::cout << "----------------------" << std::endl;
  for (auto &ele : orderBook.getAskLevels()) {
    auto &lev = ele.second;
    std::cout << lev.getPrice() << " " << lev.getQuantity()
              << lev.getOrderList().size() << std::endl;
  }
  std::cout << "----------------------" << std::endl;

  return 0;
}
