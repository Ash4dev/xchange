#include "include/Level.hpp"
#include "utils/alias/Fundamental.hpp"

#include <iterator>
#include <unordered_map>
Level::Level(Symbol symbol, Price price, Quantity quantity)
    : m_symbol{symbol}, m_price{price}, m_quantity{quantity}, m_orderList{} {}

TimeStamp Level::getActivationTime(const OrderID &orderID) {
  OrderPointer orderptr = (m_info[orderID]).order;
  return orderptr->getActivationTime();
}

TimeStamp Level::getDeactivationTime(const OrderID &orderID) {
  OrderPointer orderptr = (m_info[orderID]).order;
  return orderptr->getDeactivationTime();
}

void Level::AddOrder(Order &order) {
  assert(m_price == order.getPrice()); // ensure price matches

  if (m_info.count(order.getOrderID()) > 0) { // avoid readdition of same order
    return;
  }
  m_quantity += order.getRemainingQuantity(); // add quantity to current level

  // uses copy constructor over provided constructor to create pointer to
  // resource
  OrderPointer currentOrderPointer = std::make_shared<Order>(order);
  m_orderList.push_back(currentOrderPointer); // pass by value (copy created)
  // verify copy creation using currentOrderPointer.use_count(): ref incr by 1

  // https://stackoverflow.com/a/2678214 (iterator of latest aka last order)
  OrderList::iterator listEndIterator = std::prev(m_orderList.end());

  // store information of the order in the level
  m_info[order.getOrderID()] =
      OrderPointerInfo{currentOrderPointer, listEndIterator};
}

void Level::CancelOrder(OrderID orderID) {
  // cannot delete non-existing order in a level
  if (m_info.count(orderID) == 0) {
    return;
  }
  // obtain to be deleted order information
  const auto &[corrOrderPointer, corrOrderListIterator] = m_info.at(orderID);

  // debugging: before & after: m_quantity, ol size, info size
  m_quantity -=
      corrOrderPointer->getRemainingQuantity(); // subtract qty from level

  m_orderList.erase(corrOrderListIterator); // remove from list of orders
  m_info.erase(orderID);                    // remove information of old order
}

void Level::ModifyOrder(OrderID oldOrderID, Order &ModifiedOrder) {
  CancelOrder(oldOrderID);
  AddOrder(ModifiedOrder);
}

void Level::UpdateLevelQuantityPostMatch(Quantity filledQuantity) {
  m_quantity -= filledQuantity;
}

void Level::removeMatchedOrder(
    OrderID orderID) { // orderID of the order executed fully (no volume left)
  // find the matched order
  std::unordered_map<OrderID, OrderPointerInfo>::iterator it =
      m_info.find(orderID);
  if (it == m_info.end()) {
    return; // ignore if already removed
  }
  m_orderList.erase(
      it->second.orderListIterator); // remove info from level info
  m_info.erase(it);
}
