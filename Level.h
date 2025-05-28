#pragma once

#include <cassert>
#include <iterator>
#include <memory>
#include <unordered_map>

#include "Order.h"
#include "utils/alias/Fundamental.h"
#include "utils/alias/OrderRel.h"
#include "utils/enums/OrderTypes.h"
#include "utils/enums/Side.h"

class Level {
public:
  struct OrderPointerInfo {
    OrderPointer order{nullptr};             // store pointer to order
    OrderList::iterator orderListIterator{}; // store loc in list
  };

  Level() = default;
  Level(Symbol Symbol, Price price, Quantity quantity);

  // core level functionality
  void AddOrder(Order &order);
  void CancelOrder(OrderID orderID);
  void ModifyOrder(OrderID oldOrderID, Order &ModifiedOrder);

  // sanity maintainence functionality
  void UpdateLevelQuantityPostMatch(Quantity filledQuantity);
  void removeMatchedOrder(OrderID orderID);

  // getters
  Price getPrice() const { return m_price; }
  Quantity getQuantity() const { return m_quantity; }
  OrderList getOrderList() const { return m_orderList; }
  TimeStamp getActivationTime(const OrderID &orderID);
  TimeStamp getDeactivationTime(const OrderID &orderID);

private:
  Symbol m_symbol;
  Price m_price;
  Quantity m_quantity;   // aggregated quantity on the level
  OrderList m_orderList; // list of orders at the level
  std::unordered_map<OrderID, OrderPointerInfo> m_info; // quick info access
};
