#include "include/Preprocess.hpp"
#include "include/OrderBook.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <ostream>
#include <set>
#include <unordered_map>
#include <vector>

PreProcessor::PreProcessor(std::shared_ptr<OrderBook> &orderbookPtr,
                           bool isBidPreprocessor)
    : m_orderbookPtr(orderbookPtr) {
  int typesz = m_typeRank.size();
  m_isBidPreprocessor = isBidPreprocessor;
  m_laterProcessOrders.resize(typesz);
  m_lastFlushTime = std::chrono::system_clock::now();
}

void PreProcessor::InsertIntoPreprocessing(
    const OrderActionInfo &orderactinfo) {
  int typeId = m_typeRank[orderactinfo.orderptr->getOrderType()];
  m_laterProcessOrders[typeId].insert(orderactinfo); // add to specific multiset
  seenOrders[orderactinfo.orderptr->getOrderID()] = orderactinfo;
  PreProcessor::TryFlush(); // flush to see if PreProcessor can clean up
}

void PreProcessor::InsertIntoPreprocessing(const Order &order, bool action) {
  // orderptr, true: if add order into orderbook
  // orderptr, false: if remove order from orderbook
  OrderPointer orderptr = std::make_shared<Order>(order);
  PreProcessor::OrderActionInfo ordactinfo =
      PreProcessor::OrderActionInfo(orderptr, action);

  OrderType::OrderType otype = orderptr->getOrderType();

  // need to set deactivate time for GoodForDay orders
  // others have correctly filled activate & deactivate times
  if (otype == OrderType::OrderType::GoodForDay) {
    if (!PreProcessor::canTrade())
      return; // don't insert if market closed
    orderptr->setDeactivationTime(PreProcessor::getNextCloseTime());
  }

  PreProcessor::InsertIntoPreprocessing(ordactinfo);
}

void PreProcessor::RemoveFromPreprocessing(const OrderID &orderId) {
  // alt1: encode all info of order class into orderID (less space, poor
  // scalability) alt2: maintain unordered_map<OrderID, OrderPointer> (more
  // space, no collision) go with alt2: attribute scalable, lesser debug issues
  if (seenOrders.count(orderId) == 0)
    return;
  OrderActionInfo corrordactinfo = seenOrders[orderId];
  OrderPointer orderptr = corrordactinfo.orderptr;
  int typeId = m_typeRank[orderptr->getOrderType()];

  // if identical order add and cancel?? eliminate on spot
  if (m_laterProcessOrders[typeId].find(corrordactinfo) !=
      m_laterProcessOrders[typeId].end()) {
    m_laterProcessOrders[typeId].erase(corrordactinfo);
    seenOrders.erase(orderId); // no more use of storing seen order
    PreProcessor::TryFlush();  // flush to see if PreProcessor can clean up
    return;
  }

  // if already into orderbook, preprocess addition of cancel request
  corrordactinfo.action = false;
  seenOrders[orderId] = corrordactinfo;
  PreProcessor::InsertIntoPreprocessing(corrordactinfo);
}

void PreProcessor::ModifyInPreprocessing(const OrderID &oldID,
                                         Order &newOrder) {
  // modify not shown explicitly since linear combination
  RemoveFromPreprocessing(oldID);
  InsertIntoPreprocessing(newOrder, true);
}

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

bool PreProcessor::canMatchOrder(const OrderPointer &orderptr) {
  // action is add by default
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
    PreProcessor::RemoveFromPreprocessing(orderptr->getOrderID());
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
      PreProcessor::RemoveFromPreprocessing(orderptr->getOrderID());
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

void PreProcessor::EmptyTypeRankedOrders(
    std::multiset<OrderActionInfo> &typeRankedOrders) {

  std::vector<OrderActionInfo> insertedinWaitQueue;
  for (const OrderActionInfo &orderactinfo : typeRankedOrders) {
    // cancel order can always be inserted into orderbook
    // if to be added but can't be matched no sense in putting to wait queue
    if (orderactinfo.action &&
        !PreProcessor::canMatchOrder(orderactinfo.orderptr))
      continue;
    m_waitQueue.push(orderactinfo);
    insertedinWaitQueue.push_back(orderactinfo);
  }

  // clear these orders no longer in preprocessor but in wait queue
  for (auto &orderactinfo : insertedinWaitQueue) {
    typeRankedOrders.erase(typeRankedOrders.find(orderactinfo));
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
    auto &[topOdrptr, isadd] = m_waitQueue.front();
    m_waitQueue.pop();
    if (isadd)
      m_orderbookPtr->AddOrder(*topOdrptr);
    else
      m_orderbookPtr->CancelOrder(topOdrptr->getOrderID());
  }
}

void PreProcessor::ClearSeenOrdersWhenMatched(
    std::vector<OrderID> &matchedIDs) {
  for (auto id : matchedIDs) {
    if (!seenOrders.count(id))
      continue;
    seenOrders.erase(id);
  }
}

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

void PreProcessor::printPreProcessorStatus() {
  std::cout << "LATER QUEUES" << std::endl;

  // why does std::views::enumerate fail? no member views in std
  for (std::size_t idx = 0; idx < m_laterProcessOrders.size(); idx++) {
    auto ms = m_laterProcessOrders[idx];
    std::cout << "type: " << getType(m_rankType[idx]) << " size: " << ms.size()
              << std::endl;
    std::cout << "ORDERS: " << std::endl;
    for (auto &info : ms) {
      OrderPointer ptr = info.orderptr;
      bool action = info.action;
      std::cout << "action: " << ((action) ? "ADD" : "CANCEL")
                << " price: " << ptr->getPrice()
                << " rem qty: " << ptr->getRemainingQuantity() << std::endl;
      ptr->printTimeInfo();
    }
  }

  std::cout << "WAIT QUEUES" << std::endl;
  std::cout << "size: " << m_waitQueue.size() << std::endl;

  // print all the orders left in waitQueue
  auto copyOfWaitQueue = m_waitQueue;
  while (!copyOfWaitQueue.empty()) {
    auto [ptr, action] = copyOfWaitQueue.front();
    std::cout << "action: " << ((action) ? "ADD" : "CANCEL")
              << " price: " << ptr->getPrice()
              << " rem qty: " << ptr->getRemainingQuantity() << std::endl;
    ptr->printTimeInfo();
    copyOfWaitQueue.pop();
  }
}

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
