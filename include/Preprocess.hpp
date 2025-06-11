#pragma once

#include "include/Order.hpp"
#include "include/OrderBook.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/OrderTypes.hpp"

#include <chrono>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class PreProcessor {
public:
  PreProcessor(std::string &symbol);

  struct OrderActionInfo {
    // no point in carrying around whole order
    // pointer to it will be enough
    OrderPointer orderptr;
    bool action; // 1 -> buy

    OrderActionInfo(OrderPointer &orderptr, bool action)
        : orderptr{orderptr}, action(action) {};

    bool operator<(const OrderActionInfo &other) const {
      return ((*(this->orderptr)) < (*(other.orderptr)));
    }
  };

  std::string getType(OrderType::OrderType type);
  void printPreProcessorStatus();

  void AddOrderInOrderBook(Order &order);
  void CancelOrderFromOrderBook(const OrderID &orderId, OrderBook &orderbook);
  void ModifyOrderFromOrderBook(const OrderID &oldID, OrderBook &orderbook,
                                Order &newOrder);

  void QueueOrdersIntoWaitQueue();
  void EmptyWaitQueue(OrderBook &orderbook);
  void TryFlush(OrderBook &orderbook);

  // clear these orders from seenOrders once matched to control size
  void ClearSeenOrdersWhenMatched(std::vector<OrderID> &matchedIDs);

private:
  // m_typeRank determines index in vector (uniformity)
  static std::unordered_map<OrderType::OrderType, int> m_typeRank;
  static std::unordered_map<int, OrderType::OrderType> m_rankType;

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
