#include "include/OrderTraded.hpp"
#include "include/Trade.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderStatus.hpp"
#include "utils/enums/Side.hpp"
#include <cassert>
#include <include/Participant.hpp>
#include <vector>

Participant::ParticipantOrderInfo::ParticipantOrderInfo(
    const OrderID &orderID, const Actions::Actions &action)
    : orderID{orderID}, action{action} {
  ;
}

OrderPointer Participant::recordNonCancelOrder(
    const Actions::Actions action, const Symbol &symbol,
    const OrderType::OrderType orderType, const Side::Side side,
    const double price, const Quantity quantity,
    const ParticipantID &participantID, const std::string &activationTime,
    const std::string &deactivationTime) {

  assert(action != Actions::Actions::Cancel);

  // initially status of any order is NotProcessed
  // when an order actually enters the orderbook can its status be processing
  // when an order is seen as a trade, it will be marked as processed
  OrderPointer orderptr =
      std::make_shared<Order>(symbol, orderType, side, price, quantity,
                              participantID, activationTime, deactivationTime);
  OrderID orderID = orderptr->getOrderID();
  ParticipantOrderInfo partorderinfo = ParticipantOrderInfo(orderID, action);
  m_placedOrders[orderID] = (partorderinfo);

  m_orderComposition[orderID] = orderptr;
  return orderptr;
}

void Participant::recordTrades(const std::vector<Trade> &recentTrades) {
  for (auto &trade : recentTrades) {
    ParticipantID buyPartID = trade.getMatchedBid().participantID;
    ParticipantID sellPartID = trade.getMatchedAsk().participantID;

    if (buyPartID == m_participantID && sellPartID == m_participantID)
      return;

    if (buyPartID != m_participantID && sellPartID != m_participantID)
      return;

    m_historyOfTrades.push_back(trade);

    OrderID buyOrderID = trade.getMatchedBid().orderID;
    OrderID sellOrderID = trade.getMatchedAsk().orderID;
    OrderID matchedID =
        ((buyPartID == m_participantID) ? buyOrderID : sellOrderID);
    Side::Side side =
        ((matchedID == buyOrderID) ? Side::Side::Buy : Side::Side::Sell);

    if (m_orderComposition.count(matchedID) == 0)
      return;
    Participant::updateOrderStatus(matchedID);
    Participant::updatePortfolio(side, trade);
  }
}

void Participant::updateOrderStatus(const OrderID &matchedID) {
  if (m_orderComposition[matchedID]->isFullyFilled())
    m_orderComposition[matchedID]->setOrderStatus(
        OrderStatus::OrderStatus::Fulfilled);
}

void Participant::updatePortfolio(const Side::Side &side, const Trade &trade) {
  OrderTraded tradedOrder = ((side == Side::Side::Buy) ? trade.getMatchedBid()
                                                       : trade.getMatchedAsk());
  Amount original = m_portfolio[trade.getSymbol()];
  Amount updated =
      original +
      ((side == Side::Side::Buy) ? 1 : -1) *
          static_cast<Amount>(tradedOrder.price * tradedOrder.quantityFilled);
  m_portfolio[trade.getSymbol()] = updated;
}

ParticipantID Participant::getParticipantID() { return m_participantID; }
Portfolio Participant::getPortfolio() { return m_portfolio; }
Participant::ParticipantOrderInfo
Participant::getOrderInformation(OrderID &orderID) {
  return m_placedOrders[orderID];
}
std::vector<Trade> Participant::getHistoryOfTrades() {
  return m_historyOfTrades;
}

void Participant::setParticipantID(const ParticipantID &newID) {
  m_participantID = newID;
}
