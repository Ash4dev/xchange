#pragma once

#include "include/OrderBook.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderBookRel.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <cassert>
#include <chrono>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PreProcessor {
public:
  PreProcessor(OrderBookPointer &orderbookPtr, bool isBidPreprocessor);
  PreProcessor(OrderBookPointer &orderbookPtr, bool isBidPreprocessor,
               std::size_t pendingOrderThreshold,
               std::chrono::milliseconds pendingDurationThreshold);

  struct OrderActionInfo {
    OrderID orderID;
    OrderType::OrderType orderType;
    Actions::Actions action;

    OrderActionInfo(const OrderID &orderId, OrderType::OrderType orderType,
                    Actions::Actions action);

    static Side::Side decodeSideFromOrderID(const OrderID orderID);
    static Price decodePriceFromOrderID(const OrderID orderID);

    // follow rule of 3/5 as per need (context: constructors)
    OrderActionInfo() = default;
    bool operator<(const OrderActionInfo &other) const;
  };

  std::size_t getMaxPendingOrdersThreshold() const;
  void setMaxPendingOrdersThreshold(std::size_t threshold);

  std::chrono::milliseconds getMaxPendingDuration() const;
  void setMaxPendingDuration(std::chrono::milliseconds duration);

  std::string getType(OrderType::OrderType type);
  void printPreProcessorStatus();

  void InsertAddOrderIntoPreprocessing(const OrderPointer &orderptr);
  void InsertCancelOrderIntoPreProcessing(const OrderID &orderID,
                                          const OrderType::OrderType orderType);
  void InsertIntoPreprocessing(const OrderActionInfo &orderactinfo);
  void InsertIntoPreprocessing(const OrderPointer &orderptr,
                               Actions::Actions action);
  void RemoveFromPreprocessing(const OrderID &orderId,
                               const OrderType::OrderType &orderType);
  void ModifyInPreprocessing(const OrderID &oldID,
                             const OrderPointer &orderptr);

  bool canMatchOrder(const OrderID &orderID);
  void EmptyTypeRankedOrders(std::multiset<OrderActionInfo> &typeRankedOrders);
  void QueueOrdersIntoWaitQueue();
  void EmptyWaitQueue();
  void TryFlush();

  // clear these orders from seenOrders once matched to control size
  void ClearSeenOrdersWhenMatched();

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
  std::size_t MAX_PENDING_ORDERS_THRESHOLD;
  std::chrono::milliseconds MAX_PENDING_DURATION;

  // OrderBook as attribute as orderbook is unique for all prepro of same symbol
  // shared_ptr orderbook is shared across all prepro instances (bids, asks)
  // OrderBook ptr removed once all preprocessors removed
  // independence across symbols will also be maintained
  std::shared_ptr<OrderBook> m_orderbookPtr;
  bool m_isBidPreprocessor;
  std::vector<std::multiset<OrderActionInfo>> m_laterProcessOrders;
  std::queue<OrderActionInfo> m_waitQueue;
  std::chrono::system_clock::time_point m_lastFlushTime;

  std::unordered_set<OrderID> m_encounteredOrders;
  std::unordered_map<OrderID, OrderActionInfo>
      m_processingOrders; // red-black-tree
  std::unordered_map<OrderID, OrderPointer> m_orderComposition;

  /* synchronous way
   * need bool isInsertOrRemove acts as lock
   * when either one happens, set true once done false
   * if false, only then EmptyWaitQueue
   */
};
