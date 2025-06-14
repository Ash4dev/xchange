#pragma once

#include "include/Order.hpp"
#include "include/OrderBook.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/OrderTypes.hpp"

#include <chrono>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class PreProcessor {
public:
  PreProcessor(std::shared_ptr<OrderBook> &orderbookPtr,
               bool isBidPreprocessor);

  struct OrderActionInfo {
    // no point in carrying around whole order
    // pointer to it will be enough
    OrderPointer orderptr;
    bool action; // 1 -> buy

    OrderActionInfo(OrderPointer &orderptr, bool action)
        : orderptr{orderptr}, action(action) {};

    // if user-provided constructors exist
    // user must define a default construct explicity
    // cost me 3 hrs
    OrderActionInfo() = default;
    bool operator<(const OrderActionInfo &other) const {
      return ((*(this->orderptr)) < (*(other.orderptr)));
    }
  };

  std::string getType(OrderType::OrderType type);
  void printPreProcessorStatus();

  void InsertIntoPreprocessing(const OrderActionInfo &orderactinfo);
  void InsertIntoPreprocessing(const Order &order, bool action);
  void RemoveFromPreprocessing(const OrderID &orderId);
  void ModifyInPreprocessing(const OrderID &oldID, Order &newOrder);

  bool canMatchOrder(const OrderPointer &orderptr);
  void EmptyTypeRankedOrders(std::multiset<OrderActionInfo> &typeRankedOrders);
  void QueueOrdersIntoWaitQueue();
  void EmptyWaitQueue();
  void TryFlush();

  // clear these orders from seenOrders once matched to control size
  // NOTE: should not PreProcessor be free of responsibility once inserted?
  void ClearSeenOrdersWhenMatched(std::vector<OrderID> &matchedIDs);

private:
  // m_typeRank determines index in vector (uniformity)
  static std::unordered_map<OrderType::OrderType, int> m_typeRank;
  static std::unordered_map<int, OrderType::OrderType> m_rankType;

  // maa's idea to also check for public holidays
  static constexpr std::array<std::tuple<unsigned, unsigned, int>, 15>
      m_holidays = {{
          {26u, 1u, 2025},  // Republic Day
          {14u, 3u, 2025},  // Holi
          {31u, 3u, 2025},  // Idul Fitr
          {6u, 4u, 2025},   // Ram Navami
          {18u, 4u, 2025},  // Good Friday
          {12u, 5u, 2025},  // Buddha Purnima
          {7u, 6u, 2025},   // Bakrid
          {6u, 7u, 2025},   // Muharram
          {15u, 8u, 2025},  // Independence Day
          {16u, 8u, 2025},  // Janmashtami
          {2u, 10u, 2025},  // Gandhi Jayanti
          {2u, 10u, 2025},  // Vijaya Dashami
          {21u, 10u, 2025}, // Diwali
          {5u, 11u, 2025},  // Guru Nanak Jayanti
          {25u, 12u, 2025}  // Christmas
      }};
  static bool isHoliday(const TimeStamp &time);
  static bool canTrade();

  static TimeStamp getNextMarketTime(bool isOpen);
  static TimeStamp getNextOpenTime() { return getNextMarketTime(true); }
  static TimeStamp getNextCloseTime() { return getNextMarketTime(false); }

  // configurable
  const std::size_t MAX_PENDING_ORDERS_THRESHOLD = 3;
  const std::chrono::milliseconds MAX_PENDING_DURATION =
      std::chrono::milliseconds(100000000);

  // OrderBook as attribute as orderbook is unique for all prepro of same symbol
  // shared_ptr orderbook is shared across all prepro instances (bids, asks)
  // OrderBook ptr removed once all preprocessors removed
  // independence across symbols will also be maintained
  bool m_isBidPreprocessor;
  std::shared_ptr<OrderBook> m_orderbookPtr;
  std::vector<std::multiset<OrderActionInfo>> m_laterProcessOrders;
  std::queue<OrderActionInfo> m_waitQueue;
  std::chrono::system_clock::time_point m_lastFlushTime;
  std::unordered_map<OrderID, OrderActionInfo> seenOrders; // red-black-tree

  /* synchronous way
   * need bool isInsertOrRemove acts as lock
   * when either one happens, set true once done false
   * if false, only then EmptyWaitQueue
   */
};
