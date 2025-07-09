#include "include/Preprocess.hpp"
#include "include/OrderBook.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderBookRel.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <memory>
#include <ostream>
#include <set>
#include <unordered_map>
#include <vector>

//////////////////////////////////////////
//////// OrderActionInfo methods ////////
////////////////////////////////////////

PreProcessor::OrderActionInfo::OrderActionInfo(const OrderID &orderId,
                                               OrderType::OrderType orderType,
                                               Actions::Actions action)
    : orderID{orderId}, orderType{orderType}, action{action} {};

Side::Side
PreProcessor::OrderActionInfo::decodeSideFromOrderID(const OrderID orderID) {
  return ((orderID & 0x1)
              ? Side::Side::Buy
              : Side::Side::Sell); // side is the last bit of orderID
}

Price PreProcessor::OrderActionInfo::decodePriceFromOrderID(
    const OrderID orderID) {
  auto price = (orderID >> 1) & ((static_cast<OrderID>(1) << 31) - 1);
  return price;
}

bool PreProcessor::OrderActionInfo::operator<(
    const OrderActionInfo &other) const {
  OrderID thisID = this->orderID;
  OrderID otherID = other.orderID;

  // | 63-32 (timestamp) | 31-1 (price) | 0 (side) |
  Side::Side thisSide = decodeSideFromOrderID(thisID);
  assert(thisSide == decodeSideFromOrderID(otherID));

  Price thisPrice = decodePriceFromOrderID(thisID);
  Price otherPrice = decodePriceFromOrderID(otherID);

  auto thisTime = (thisID >> 32);
  auto otherTime = (otherID >> 32);

  if (thisPrice == otherPrice) {
    return thisTime <= otherTime;
  }

  if (thisSide == Side::Side::Buy) {
    return thisPrice > otherPrice;
  } else {
    return thisPrice < otherPrice;
  }
}

//////////////////////////////////////////
//////// PreProcessor constructors //////
////////////////////////////////////////

PreProcessor::PreProcessor(OrderBookPointer &orderbookPtr,
                           bool isBidPreprocessor)
    : m_orderbookPtr{orderbookPtr}, m_isBidPreprocessor{isBidPreprocessor} {
  MAX_PENDING_ORDERS_THRESHOLD = static_cast<std::size_t>(3);
  MAX_PENDING_DURATION = static_cast<std::chrono::milliseconds>(100);
}

PreProcessor::PreProcessor(std::shared_ptr<OrderBook> &orderbookPtr,
                           bool isBidPreprocessor,
                           std::size_t pendingOrdersThreshold,
                           std::chrono::milliseconds pendingDurationThreshold)
    : m_orderbookPtr(orderbookPtr) {
  int typesz = m_typeRank.size();
  m_isBidPreprocessor = isBidPreprocessor;
  m_laterProcessOrders.resize(typesz);
  m_lastFlushTime = std::chrono::system_clock::now();

  MAX_PENDING_ORDERS_THRESHOLD = pendingOrdersThreshold;
  MAX_PENDING_DURATION = pendingDurationThreshold;
}

/*
////////////////////////////////////////////////
Core functionality: Insert, Remove, Modify in PreProcessing
////////////////////////////////////////////////
*/

void PreProcessor::InsertIntoPreprocessing(
    const OrderActionInfo &orderactinfo) {
  int typeId = m_typeRank[orderactinfo.orderType];
  m_laterProcessOrders[typeId].insert(orderactinfo); // add to specific multiset
  m_encounteredOrders.insert(orderactinfo.orderID);

  m_processingOrders[orderactinfo.orderID] = orderactinfo;
  PreProcessor::TryFlush(); // flush to see if PreProcessor can clean up
}

void PreProcessor::InsertAddOrderIntoPreprocessing(
    const OrderPointer &orderptr) {
  OrderID currOrderId = orderptr->getOrderID();
  if (m_orderComposition.count(currOrderId) != 0)
    return;

  if (orderptr->getOrderType() == OrderType::OrderType::GoodForDay) {
    if (!PreProcessor::canTrade())
      return; // don't insert if market closed
    orderptr->setDeactivationTime(PreProcessor::getNextCloseTime());
  }
  m_orderComposition[currOrderId] = orderptr;

  OrderActionInfo orderactinfo = PreProcessor::OrderActionInfo(
      currOrderId, orderptr->getOrderType(), Actions::Actions::Add);
  PreProcessor::InsertIntoPreprocessing(orderactinfo);
}

void PreProcessor::InsertCancelOrderIntoPreProcessing(
    const OrderID &orderID, const OrderType::OrderType orderType) {
  OrderActionInfo orderactinfo = PreProcessor::OrderActionInfo(
      orderID, orderType, Actions::Actions::Cancel);
  PreProcessor::InsertIntoPreprocessing(orderactinfo);
}

void PreProcessor::RemoveFromPreprocessing(
    const OrderID &orderId, const OrderType::OrderType &orderType) {

  // cannot cancel an cancel order (need to replace original order again)

  // never ever seen this order
  if (m_encounteredOrders.count(orderId) == 0)
    return;

  if (m_processingOrders.count(orderId) == 0) {
    // if already into orderbook, preprocess reverse of the original request
    PreProcessor::InsertCancelOrderIntoPreProcessing(orderId, orderType);
    return;
  }

  // order still being processed
  OrderActionInfo corrordactinfo = m_processingOrders[orderId];
  int typeId = m_typeRank[orderType];

  // if identical order add and cancel?? eliminate on spot
  auto it = m_laterProcessOrders[typeId].find(corrordactinfo);
  if (it != m_laterProcessOrders[typeId].end()) {
    m_laterProcessOrders[typeId].erase(it);
    m_processingOrders.erase(orderId); // no more use of storing seen order
    PreProcessor::TryFlush(); // flush to see if PreProcessor can clean up
    return;
  }
}

void PreProcessor::ModifyInPreprocessing(const OrderID &oldID,
                                         const OrderPointer &orderptr) {
  // modify not shown explicitly since linear combination

  // symbol, orderType cannot be modified
  RemoveFromPreprocessing(oldID, orderptr->getOrderType());
  InsertAddOrderIntoPreprocessing(orderptr);
}

/*
////////////////////////////////////////////////
Flushing out, Queueing in Wait Queue, EmptyWaitQueue
////////////////////////////////////////////////
*/

void PreProcessor::TryFlush() {
  // qty buffered
  std::size_t totalBufferedOrders = 0;
  for (const std::multiset<OrderActionInfo> &typeRankedOrders :
       m_laterProcessOrders) {
    totalBufferedOrders += typeRankedOrders.size();
  }

  // time interval
  auto now = std::chrono::system_clock::now();
  auto durationSinceLastFlush =
      std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                            m_lastFlushTime);

  // hybrid flush model: qty or time interval (synchronous)
  if (totalBufferedOrders >= MAX_PENDING_ORDERS_THRESHOLD ||
      durationSinceLastFlush >= MAX_PENDING_DURATION) {
    QueueOrdersIntoWaitQueue();
    EmptyWaitQueue();
    m_lastFlushTime = now;
  }
}

void PreProcessor::QueueOrdersIntoWaitQueue() {
  auto now = std::chrono::system_clock::now();
  auto now_min = std::chrono::time_point_cast<std::chrono::minutes>(now);

  // handle MarketOnClose & MarketOnOpen order types first
  if (now_min == PreProcessor::getNextCloseTime())
    PreProcessor::EmptyTypeRankedOrders(
        m_laterProcessOrders[m_typeRank[OrderType::OrderType::MarketOnClose]]);
  if (now_min == PreProcessor::getNextOpenTime())
    PreProcessor::EmptyTypeRankedOrders(
        m_laterProcessOrders[m_typeRank[OrderType::OrderType::MarketOnOpen]]);

  for (std::size_t i = 0; i + 2 < m_laterProcessOrders.size(); i++) {
    PreProcessor::EmptyTypeRankedOrders(m_laterProcessOrders[i]);
  }
}

void PreProcessor::EmptyTypeRankedOrders(
    std::multiset<OrderActionInfo> &typeRankedOrders) {
  if (typeRankedOrders.empty())
    return;

  std::multiset<PreProcessor::OrderActionInfo> copyOfTypeRankedOrders =
      typeRankedOrders;

  std::vector<PreProcessor::OrderActionInfo> insertedinWaitQueue;
  for (const PreProcessor::OrderActionInfo &orderactinfo :
       copyOfTypeRankedOrders) {
    // cancel order can always be inserted into orderbook
    // if to be added but can't be matched no sense in putting to wait queue

    // use of canMatchOrder has RemoveFromPreprocessing embedded on looping with
    // typeRankedOrders can affect size of typeRankedOrders (decreases) -> cause
    // of seg fault
    // what causes: https://stackoverflow.com/a/3719031/19817062 (gdb)
    // why caused: https://stackoverflow.com/a/3719076/19817062 (valgrind)
    // valgrind --leak-check=full -s ./bin/orderbook (-s: show error list)
    // https://docs.google.com/document/d/1OhOvXoePXU5l7TkWUzPs-1HU424P9buZekG7lluupq8/edit?usp=sharing

    // ATTEMPT 1: failed loop on typeRankedOrders itself
    // if (typeRankedOrders.find(orderactinfo) == typeRankedOrders.end())
    //   continue;

    // ATTEMPT 2: successful loop on copyOfTypeRankedOrders (SIZE <=
    // MAX_PENDING_ORDERS_THRESHOLD)
    if (typeRankedOrders.find(orderactinfo) == typeRankedOrders.end())
      continue;

    if (orderactinfo.action == Actions::Actions::Add) {
      if (!PreProcessor::canMatchOrder(orderactinfo.orderID))
        continue;
    }
    m_waitQueue.push(orderactinfo);
    insertedinWaitQueue.push_back(orderactinfo);
  }

  // clear these orders no longer in preprocessor but in wait queue
  for (auto &orderactinfo : insertedinWaitQueue) {
    auto it = typeRankedOrders.find(orderactinfo);
    if (it == typeRankedOrders.end())
      continue;
    typeRankedOrders.erase(it);
  }
}

void PreProcessor::EmptyWaitQueue() {
  // manual override for experimentation purpose
  // since today (Jun 14 Saturday) market is closed
  // test: successful

  if (!PreProcessor::canTrade()) { // can't send into orderbook if closed
    // std::cout << "Market Closed :(" << std::endl;
    return;
  }
  while (!m_waitQueue.empty()) {
    auto &[orderID, _, action] = m_waitQueue.front();
    m_waitQueue.pop();
    if (action == Actions::Actions::Add &&
        m_orderComposition.count(orderID) > 0)
      m_orderbookPtr->AddOrder(*m_orderComposition[orderID]);
    else
      m_orderbookPtr->CancelOrder(orderID);
  }
  PreProcessor::ClearSeenOrdersWhenMatched();
}

void PreProcessor::ClearSeenOrdersWhenMatched() {

  std::vector<Trade> trades = m_orderbookPtr->getTrades();

  // iterate only on the latest trades (get appended)
  for (auto it = trades.rbegin(); it != trades.rend(); it++) {
    const auto &trade = *it;
    OrderID bidId = trade.getMatchedBid().orderID;
    OrderID askId = trade.getMatchedAsk().orderID;

    // trade cannot happen on the basis of a cancel order hence it must be add
    // stop once an trade has been encountered and cleared previously
    if ((m_orderComposition.count(bidId) == 0) ||
        (m_orderComposition.count(askId) == 0))
      return;

    if (m_orderComposition.count(bidId) > 0 &&
        m_orderComposition[bidId]->getRemainingQuantity() == 0) {
      if (m_processingOrders.count(bidId) > 0)
        m_processingOrders.erase(bidId);
      m_orderComposition.erase(bidId);
    }

    if (m_orderComposition.count(askId) > 0 &&
        m_orderComposition[askId]->getRemainingQuantity() == 0) {
      if (m_processingOrders.count(askId) > 0)
        m_processingOrders.erase(askId);
      m_orderComposition.erase(askId);
    }
  }
}

/*
////////////////////////////////////////////////
Flushing etc UTILITIES
////////////////////////////////////////////////
*/

Quantity qtyAvailableForMatch(const OrderPointer &orderptr,
                              const OrderBook &orderbook) {
  Price price = orderptr->getPrice();
  Side::Side side = orderptr->getSide();

  Quantity qtyAvailable = 0;
  if (side == Side::Side::Buy) {
    // For BUY orders: match against ASK levels (ask_price <= bid_price)
    const auto &levels = orderbook.getAskLevels();
    for (const auto &[ask_price, level] : levels) {
      if (ask_price > price)
        break; // Stop if ask price exceeds bid price
      qtyAvailable += level->getQuantity();
    }
  } else {
    // For SELL orders: match against BID levels (bid_price >= ask_price)
    const auto &levels = orderbook.getBidLevels();
    for (const auto &[bid_price, level] : levels) {
      if (bid_price < price)
        break; // Stop if bid price below ask price
      qtyAvailable += level->getQuantity();
    }
  }
  return qtyAvailable;
}

bool PreProcessor::canMatchOrder(const OrderID &orderID) {
  // action is add by default
  if (m_orderComposition.count(orderID) == 0)
    return false;
  const OrderPointer &orderptr = m_orderComposition[orderID];
  OrderType::OrderType otype = orderptr->getOrderType();

  // no conditions on these order types
  if (otype == OrderType::OrderType::Market ||
      otype == OrderType::OrderType::GoodTillCancel)
    return true;

  auto now = std::chrono::system_clock::now();

  if (otype == OrderType::OrderType::GoodAfterTime) {
    return (now >= orderptr->getActivationTime());
  }

  if (otype == OrderType::OrderType::GoodForDay ||
      otype == OrderType::OrderType::GoodTillDate) {
    if (now < orderptr->getDeactivationTime()) // fine if not deactivated
      return true;

    // deactivated no more point keeping it
    PreProcessor::RemoveFromPreprocessing(orderptr->getOrderID(),
                                          orderptr->getOrderType());
    return false;
  }

  Quantity qtyAvailable =
      qtyAvailableForMatch(orderptr, *PreProcessor::m_orderbookPtr);
  if (otype == OrderType::OrderType::FillOrKill ||
      otype == OrderType::OrderType::AllOrNone) {
    // more quantity should be available on the other side
    if (qtyAvailable >= orderptr->getRemainingQuantity())
      return true;

    // no point in keeping fok order if not executable now
    if (otype == OrderType::OrderType::FillOrKill)
      PreProcessor::RemoveFromPreprocessing(orderptr->getOrderID(),
                                            orderptr->getOrderType());
    return false;
  }
  if (otype == OrderType::OrderType::ImmediateOrCancel) {
    Quantity finalQuantity =
        std::min(qtyAvailable, orderptr->getRemainingQuantity());
    orderptr->setQuantity(finalQuantity); // rest is effectively cancelled
    return true;
  }

  return false; // don't add prevents wrong matching atleast
}

// check if the market is open currently (between 9:15 to 15:30)
bool PreProcessor::canTrade() {
  using namespace std::chrono;
  auto now = system_clock::now();

  // Check if weekend
  auto today = floor<days>(now);
  weekday dow{today};
  if (dow == Saturday || dow == Sunday) {
    return false;
  }

  // Check if holiday
  if (isHoliday(now)) {
    return false;
  }

  // Check market hours (09:15-15:30)
  auto tod = now - today;
  auto market_open = hours(9) + minutes(15);
  auto market_close = hours(15) + minutes(30);

  return tod >= market_open && tod < market_close;
}

bool PreProcessor::isHoliday(const TimeStamp &time) {
  using namespace std::chrono;
  auto dp = floor<days>(time);
  year_month_day ymd{dp};

  for (const auto &[d, m, y] : m_holidays) {
    if (ymd.day() == day{d} && ymd.month() == month{m} &&
        ymd.year() == year{y}) {
      return true;
    }
  }
  return false;
}

TimeStamp PreProcessor::getNextMarketTime(bool isOpen) {
  using namespace std::chrono;

  constexpr auto market_open = hours(9) + minutes(15);
  constexpr auto market_close = hours(15) + minutes(30);

  auto now = system_clock::now();
  auto today = floor<days>(now);
  auto target_time = today + (isOpen ? market_open : market_close);

  // Adjust for weekends
  // auto dow = weekday(target_time); type-casting error
  auto sys_days = floor<days>(target_time); // First convert to days precision
  weekday dow{sys_days};                    // Then construct weekday from days
  if (dow == Saturday)
    target_time += days(2);
  else if (dow == Sunday)
    target_time += days(1);

  // If time already passed today, move to next weekday
  if (target_time < now) {
    target_time += days(1);
    auto sys_days = floor<days>(target_time); // First convert to days precision
    weekday dow{sys_days}; // Then construct weekday from days
    if (dow == Saturday)
      target_time += days(2);
    else if (dow == Sunday)
      target_time += days(1);
  }

  return target_time;
}
/*
////////////////////////////////////////////////
Printing Debugging UTILITIES
////////////////////////////////////////////////
*/

void PreProcessor::printPreProcessorStatus() {
  std::cout << "LATER QUEUES" << std::endl;

  // why does std::views::enumerate fail? no member views in std
  for (std::size_t idx = 0; idx < m_laterProcessOrders.size(); idx++) {
    auto ms = m_laterProcessOrders[idx];
    std::cout << "type: " << getType(m_rankType[idx]) << " size: " << ms.size()
              << std::endl;
    std::cout << "ORDERS: " << std::endl;
    for (auto &info : ms) {

      OrderID orderID = info.orderID;
      Actions::Actions action = info.action;
      std::cout << "action: "
                << ((action == Actions::Actions::Add)
                        ? "ADD"
                        : ((action == Actions::Actions::Cancel) ? "CANCEL"
                                                                : "MODIFY"))
                << " price: "
                << PreProcessor::OrderActionInfo::decodePriceFromOrderID(
                       orderID)
                << std::endl;
    }
  }

  std::cout << "WAIT QUEUES" << std::endl;
  std::cout << "size: " << m_waitQueue.size() << std::endl;

  // print all the orders left in waitQueue
  auto copyOfWaitQueue = m_waitQueue;
  while (!copyOfWaitQueue.empty()) {
    auto [orderID, _, action] = copyOfWaitQueue.front();
    std::cout << "action: "
              << ((action == Actions::Actions::Add)
                      ? "ADD"
                      : ((action == Actions::Actions::Cancel) ? "CANCEL"
                                                              : "MODIFY"))
              << " price: "
              << PreProcessor::OrderActionInfo::decodePriceFromOrderID(orderID)
              << std::endl;

    copyOfWaitQueue.pop();
  }
}
/*
////////////////////////////////////////////////
Preprocessor Constants (Static DS)
////////////////////////////////////////////////
*/

std::unordered_map<OrderType::OrderType, int> PreProcessor::m_typeRank = {
    // ordered this way to minimize waiting time for priority orders
    {OrderType::OrderType::Market, 0},
    {OrderType::OrderType::FillOrKill, 1},
    {OrderType::OrderType::ImmediateOrCancel, 2},
    {OrderType::OrderType::GoodAfterTime, 3},
    {OrderType::OrderType::GoodForDay, 4},
    {OrderType::OrderType::GoodTillDate, 5},
    {OrderType::OrderType::AllOrNone, 6},
    {OrderType::OrderType::GoodTillCancel, 7},
    // normally loop till 7, push if cond satisfied (special types)
    {OrderType::OrderType::MarketOnOpen, 8},
    {OrderType::OrderType::MarketOnClose, 9}};

std::unordered_map<int, OrderType::OrderType> PreProcessor::m_rankType = {
    {0, OrderType::OrderType::Market},
    {1, OrderType::OrderType::FillOrKill},
    {2, OrderType::OrderType::ImmediateOrCancel},
    {3, OrderType::OrderType::GoodAfterTime},
    {4, OrderType::OrderType::GoodForDay},
    {5, OrderType::OrderType::GoodTillDate},
    {6, OrderType::OrderType::AllOrNone},
    {7, OrderType::OrderType::GoodTillCancel},
    {8, OrderType::OrderType::MarketOnOpen},
    {9, OrderType::OrderType::MarketOnClose}};

std::string PreProcessor::getType(OrderType::OrderType type) {
  switch (type) {
  case OrderType::OrderType::Market:
    return "Market";
  case OrderType::OrderType::FillOrKill:
    return "FillOrKill";
  case OrderType::OrderType::ImmediateOrCancel:
    return "ImmediateOrCancel";
  case OrderType::OrderType::GoodAfterTime:
    return "GoodAfterTime";
  case OrderType::OrderType::GoodForDay:
    return "GoodForDay";
  case OrderType::OrderType::GoodTillDate:
    return "GoodTillDate";
  case OrderType::OrderType::AllOrNone:
    return "AllOrNone";
  case OrderType::OrderType::GoodTillCancel:
    return "GoodTillCancel";
  case OrderType::OrderType::MarketOnOpen:
    return "MarketOnOpen";
  case OrderType::OrderType::MarketOnClose:
    return "MarketOnClose";
  default:
    return "Unknown";
  }
}
