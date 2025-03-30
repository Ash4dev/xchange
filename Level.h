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
    OrderPointer order{nullptr};
    OrderList::iterator orderListIterator{};
  };

  Level() = default;
  Level(Symbol Symbol, Price price, Quantity quantity);
  //  Level(Price price, Quantity quantity)
  //    : m_price{price}, m_quantity{quantity}, m_orderList{} {}

  void AddOrder(Order &order);
  void CancelOrder(OrderID orderID);
  void ModifyOrder(OrderID oldOrderID, Order &ModifiedOrder);

  void UpdateLevelQuantityPostMatch(Quantity filledQuantity);
  void removeMatchedOrder(OrderID orderID);

  Price getPrice() const { return m_price; }
  Quantity getQuantity() const { return m_quantity; }
  OrderList getOrderList() const { return m_orderList; }

private:
  Symbol m_symbol;
  Price m_price;
  Quantity m_quantity;
  OrderList m_orderList;
  std::unordered_map<OrderID, OrderPointerInfo> m_info;
};
