#include "Preprocess.h"
#include "OrderBook.h"
#include "utils/alias/OrderRel.h"
#include "utils/enums/OrderTypes.h"
#include <memory>
#include <set>
#include <unordered_map>

PreProcessor::PreProcessor() {
  int typesz = m_typeRank.size();
  m_laterProcessOrders.resize(typesz);
  m_lastFlushTime = std::chrono::system_clock::now();
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

void PreProcessor::InsertOrderInOrderBook(Order &order) {
  OrderPointer orderptr = std::make_shared<Order>(order);
  int typeId = m_typeRank[orderptr->getOrderType()]; // get order rank
  OrderActionInfo ordactinfo = OrderActionInfo(orderptr, true);
  m_laterProcessOrders[typeId].insert(ordactinfo); // add to specific vector
  seenOrders[orderptr->getOrderID()] = orderptr;
}

void PreProcessor::RemoveOrderFromOrderBook(OrderID &orderId,
                                            OrderBook &orderbook) {
  // alt1: encode all info of order class into orderID (less space, poor
  // scalability) alt2: maintain unordered_map<OrderID, OrderPointer> (more
  // space, no collision) go with alt2: attribute scalable, lesser debug issues
  if (seenOrders.count(orderId) == 0)
    return;
  OrderPointer orderptr = seenOrders[orderId];
  int typeId = m_typeRank[orderptr->getOrderType()];
  OrderActionInfo ordactinfo = OrderActionInfo(orderptr, false);
  m_laterProcessOrders[typeId].insert(ordactinfo); // insert to vector
  seenOrders.erase(orderId); // no more use of storing seen order
}

void PreProcessor::QueueOrdersIntoWaitQueue() {
  // TODO: add special condition for MarketOnClose & MarketOnOpen
  // integeate it QueueOrdersIntoWaitQueue & TryFlush
  for (const std::multiset<OrderActionInfo> &typeRankedOrders :
       m_laterProcessOrders) {
    for (const OrderActionInfo &orderactinfo : typeRankedOrders) {
      m_waitQueue.push(orderactinfo);
    }
  }
}

void PreProcessor::EmptyWaitQueue(OrderBook &orderbook) {
  assert(m_symbol == orderbook.getSymbol()); // ensure symbol same

  while (!m_waitQueue.empty()) {
    auto &[topOdrptr, isadd] = m_waitQueue.front();
    m_waitQueue.pop();
    if (isadd)
      orderbook.AddOrder(*topOdrptr);
    else
      orderbook.CancelOrder(topOdrptr->getOrderID());
  }
}

void PreProcessor::TryFlush(OrderBook &orderbook) {
  // qty buffered
  int totalBufferedOrders = 0;
  for (const std::multiset<OrderActionInfo> typeRankedOrders :
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
    EmptyWaitQueue(orderbook);
    m_lastFlushTime = now;
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
