#include "include/Preprocess.hpp"
#include "include/OrderBook.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/OrderTypes.hpp"

#include <cstddef>
#include <memory>
#include <ostream>
#include <set>
#include <unordered_map>

PreProcessor::PreProcessor(std::string &symbol) : m_symbol(symbol) {
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

void PreProcessor::InsertIntoPreprocessing(
    const OrderActionInfo &orderactinfo) {
  int typeId = m_typeRank[orderactinfo.orderptr->getOrderType()];
  m_laterProcessOrders[typeId].insert(orderactinfo); // add to specific multiset
  seenOrders[orderactinfo.orderptr->getOrderID()] = orderactinfo;
}

void PreProcessor::InsertIntoPreprocessing(const Order &order, bool action) {
  // orderptr, true: if add order into orderbook
  // orderptr, false: if remove order from orderbook
  OrderPointer orderptr = std::make_shared<Order>(order);
  PreProcessor::OrderActionInfo ordactinfo =
      PreProcessor::OrderActionInfo(orderptr, action);
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
  if (m_waitQueue.empty())
    return;
  auto [ptr, action] = m_waitQueue.front();
  std::cout << "action: " << ((action) ? "ADD" : "CANCEL")
            << " price: " << ptr->getPrice()
            << " rem qty: " << ptr->getRemainingQuantity() << std::endl;
  ptr->printTimeInfo();
}

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
