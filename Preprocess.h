#pragma once

#include "Order.h"
#include "Trade.h"
#include "utils/enums/OrderTypes.h"
#include <chrono>
#include <optional>
#include <queue>
#include <set>
#include <unordered_map>

class PreProcessor {
public:
  PreProcessor() = default;

  void QueueOrder(Order &order);
  void EmptyWaitQueue();
  void InsertOrderInOrderBook(Order &order);
  void RemoveOrderFromOrderBook(Order &order);

private:
  static std::chrono::system_clock m_sysClock;
  static std::unordered_map<OrderType::OrderType, int> m_typeRank;
  std::vector<std::multiset<Order>> m_instantProcessOrders;
  std::unordered_map<OrderType::OrderType, std::multiset<Order>>
      m_laterProcessOrders;
  std::queue<Order> m_waitQueue;

  /* synchronous way
   * need bool isInsertOrRemove acts as lock
   * when either one happens, set true once done false
   * if false, only then EmptyWaitQueue
   */
};
