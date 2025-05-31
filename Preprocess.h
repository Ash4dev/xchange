#pragma once

#include "Order.h"
#include "OrderBook.h"
#include "Trade.h"
#include "utils/alias/OrderRel.h"
#include "utils/enums/OrderTypes.h"
#include <chrono>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class PreProcessor {
public:
  PreProcessor();

  struct OrderActionInfo {
    // no point in carrying around whole order
    // pointer to it will be enough
    OrderPointer orderptr;
    bool is_add;

    OrderActionInfo(OrderPointer &orderptr, bool insert)
        : orderptr{orderptr}, is_add(insert) {};
  };

  // named from the context of user of end point
  void InsertOrderInOrderBook(Order &order);
  void RemoveOrderFromOrderBook(OrderID &orderId, OrderBook &orderbook);

  void QueueOrdersIntoWaitQueue();
  void EmptyWaitQueue(OrderBook &orderbook);
  void TryFlush(OrderBook &orderbook);

  // clear these orders from seenOrders once matched to control size
  void ClearSeenOrdersWhenMatched(std::vector<OrderID> &matchedIDs);

private:
  // m_typeRank determines index in vector (uniformity)
  static std::unordered_map<OrderType::OrderType, int> m_typeRank;

  const size_t MAX_PENDING_ORDERS_THRESHOLD = 50;
  const std::chrono::milliseconds MAX_PENDING_DURATION =
      std::chrono::milliseconds(1000);
  std::chrono::system_clock::time_point m_lastFlushTime;

  std::string m_symbol;
  std::vector<std::multiset<OrderActionInfo>> m_laterProcessOrders;
  std::queue<OrderActionInfo> m_waitQueue;

  // understand red-black-tree implementation for (un)ordered map
  // why O(1) query, insertion, removal?
  std::unordered_map<OrderID, OrderPointer> seenOrders;

  /* synchronous way
   * need bool isInsertOrRemove acts as lock
   * when either one happens, set true once done false
   * if false, only then EmptyWaitQueue
   */
};
