#include "Level.h"
#include "utils/alias/Fundamental.h"

#include <unordered_map>

void Level::AddOrder(Order &order) {
  assert(m_price == order.getPrice());

  if (m_info.count(order.getOrderID()) > 0) {
    return;
  }
  m_quantity += order.getRemainingQuantity();
  // uses copy constructor over provided constructor
  OrderPointer currentOrderPointer = std::make_shared<Order>(order);
  m_orderList.push_back(currentOrderPointer);

  // https://stackoverflow.com/a/2678214
  OrderList::iterator listEndIterator = std::prev(m_orderList.end());
  m_info[order.getOrderID()] =
      OrderPointerInfo{currentOrderPointer, listEndIterator};
}

void Level::CancelOrder(OrderID orderID) {
  if (m_info.count(orderID) == 0) {
    return;
  }
  const auto &[corrOrderPointer, corrOrderListIterator] = m_info.at(orderID);

  m_quantity -= corrOrderPointer->getRemainingQuantity();
  m_orderList.erase(corrOrderListIterator);
  m_info.erase(orderID);
}

void Level::ModifyOrder(OrderID oldOrderID, Order &ModifiedOrder) {
  CancelOrder(oldOrderID);
  AddOrder(ModifiedOrder);
}

void Level::removeMatchedOrder(OrderID orderID) {
  std::unordered_map<OrderID, OrderPointerInfo>::iterator it =
      m_info.find(orderID);
  if (it == m_info.end()) {
    return;
  }
  m_orderList.erase(it->second.orderListIterator);
  m_info.erase(it);
}
