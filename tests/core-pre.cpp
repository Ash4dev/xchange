#include <iostream>
#include <memory>
#include <ostream>

#include "include/Order.hpp"
#include "include/OrderBook.hpp"
#include "include/Preprocess.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

int main() {

  std::string symbol = "A";

  OrderBook orderBook = OrderBook(symbol);
  std::shared_ptr<OrderBook> orderbookPtr =
      std::make_shared<OrderBook>(orderBook);

  // create separate processor for buy and sell
  // sorting condition different
  PreProcessor bidpreProcessor = PreProcessor(orderbookPtr, true);
  PreProcessor askpreProcessor = PreProcessor(orderbookPtr, false);

  // by mistake bid order inserted in ask PreProcessor (need mechanism)

  // PreProcessor && orderBook must stay connected somehow else
  // not able to do insert and stuff

  // can insert 2 orders in a single type on a single side
  Order o1 = Order(symbol, OrderType::OrderType::GoodTillCancel,
                   Side::Side::Buy, 90.39, 20);
  bidpreProcessor.InsertIntoPreprocessing(o1, true);
  Order o12 = Order(symbol, OrderType::OrderType::GoodTillCancel,
                    Side::Side::Buy, 90.49, 20);
  bidpreProcessor.InsertIntoPreprocessing(o12, true);

  // can insert different order on other side
  Order o2 = Order(symbol, OrderType::OrderType::GoodTillCancel,
                   Side::Side::Sell, 93.04, 5);
  askpreProcessor.InsertIntoPreprocessing(o2, true);

  // can re-insert previous side
  Order o3 = Order(symbol, OrderType::OrderType::GoodAfterTime, Side::Side::Buy,
                   96.15, 77, "04-06-2025 13:04:26");
  bidpreProcessor.InsertIntoPreprocessing(o3, true);

  // just for fun (redundant)
  Order o4 = Order("A", OrderType::OrderType::MarketOnOpen, Side::Side::Sell,
                   99.86, 100);
  askpreProcessor.InsertIntoPreprocessing(o4, true);

  // can add & remove previous
  Order o5 =
      Order("A", OrderType::OrderType::FillOrKill, Side::Side::Sell, 92.43, 43);
  askpreProcessor.InsertIntoPreprocessing(o5, true);
  askpreProcessor.RemoveFromPreprocessing(o4.getOrderID());

  // not remove non-inserted order
  Order o5a =
      Order("A", OrderType::OrderType::FillOrKill, Side::Side::Sell, 98.22, 43);
  askpreProcessor.RemoveFromPreprocessing(o5a.getOrderID());

  Order o6 = Order("A", OrderType::OrderType::ImmediateOrCancel,
                   Side::Side::Buy, 97.37, 21);
  bidpreProcessor.InsertIntoPreprocessing(o6, true);

  // no re-deletion of once removed order
  bidpreProcessor.RemoveFromPreprocessing(o4.getOrderID());
  // modify removes prev & inserts new
  bidpreProcessor.ModifyInPreprocessing(o6.getOrderID(), o4); // modify

  std::cout << "BIDS" << std::endl;
  bidpreProcessor.printPreProcessorStatus();
  std::cout << std::endl;
  std::cout << "ASKS" << std::endl;
  askpreProcessor.printPreProcessorStatus();
  std::cout << std::endl;

  std::cout << "ORDERBOOK STATE" << std::endl;
  orderbookPtr->printOrderBookState();
  std::cout << std::endl;

  std::cout << "BIDS" << std::endl;
  bidpreProcessor.printPreProcessorStatus();
  std::cout << std::endl;
  std::cout << "ASKS" << std::endl;
  askpreProcessor.printPreProcessorStatus();
  std::cout << std::endl;

  return 0;
}
