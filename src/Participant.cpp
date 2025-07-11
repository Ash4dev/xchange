#include "include/OrderTraded.hpp"
#include "include/Trade.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderStatus.hpp"
#include "utils/enums/Side.hpp"
#include <cassert>
#include <cstddef>
#include <include/Participant.hpp>
#include <vector>

std::size_t Participant::lastProcessedTradeIndex = 0;

Participant::ParticipantOrderInfo::ParticipantOrderInfo(
    const OrderID &orderID, const Actions::Actions &action,
    const Symbol &symbol, const OrderType::OrderType &otype,
    const Side::Side &side)
    : orderID{orderID}, action{action}, symbol{symbol}, otype{otype},
      side{side} {}

void Participant::recordCancelOrder(const OrderID &orderID) {
  if (!isParticularOrderPlacedByParticipant(orderID))
    return;
  m_placedOrders.erase(orderID);
  m_orderComposition.erase(orderID);
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
  ParticipantOrderInfo partorderinfo =
      ParticipantOrderInfo(orderID, action, symbol, orderType, side);
  m_placedOrders[orderID] = (partorderinfo);

  m_orderComposition[orderID] = orderptr;
  return orderptr;
}

void Participant::recordTrades(const std::vector<Trade> &trades) {

  std::size_t validTrades = 0;
  for (std::size_t idx = Participant::lastProcessedTradeIndex;
       idx < trades.size(); idx++) {
    Trade trade = trades[idx];
    ParticipantID buyPartID = trade.getMatchedBid().participantID;
    ParticipantID sellPartID = trade.getMatchedAsk().participantID;

    if (buyPartID == m_participantID && sellPartID == m_participantID)
      continue;

    if (buyPartID != m_participantID && sellPartID != m_participantID)
      continue;

    validTrades++;
    m_historyOfTrades.push_back(trade);

    OrderID buyOrderID = trade.getMatchedBid().orderID;
    OrderID sellOrderID = trade.getMatchedAsk().orderID;
    OrderID matchedID =
        ((buyPartID == m_participantID) ? buyOrderID : sellOrderID);
    Side::Side side =
        ((matchedID == buyOrderID) ? Side::Side::Buy : Side::Side::Sell);

    if (m_orderComposition.count(matchedID) == 0)
      continue;
    Participant::updateOrderStatus(matchedID);
    Participant::updatePortfolio(side, trade);
  }
  Participant::lastProcessedTradeIndex += validTrades;
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

ParticipantID Participant::getParticipantID() const { return m_participantID; }
Portfolio Participant::getPortfolio() const { return m_portfolio; }

double Participant::getValuationOfSymbol(const Symbol &symbol) const {
  if (!m_portfolio.contains(symbol))
    return static_cast<double>(0);
  return static_cast<double>(m_portfolio.at(symbol));
}

double Participant::getValuationOfPortfolio() const {
  Amount amt = 0;
  for (auto [symbol, symbolVal] : m_portfolio)
    amt += symbolVal;
  return static_cast<double>(amt);
}

Participant::ParticipantOrderInfo
Participant::getOrderInformation(const OrderID &orderID) const {
  return m_placedOrders.at(orderID);
}

bool Participant::isParticularOrderPlacedByParticipant(
    const OrderID &orderID) const {
  return (m_placedOrders.contains(orderID) &&
          m_orderComposition.contains(orderID));
}

OrderPointer Participant::getOrder(const OrderID &orderId) const {
  if (!m_orderComposition.contains(orderId))
    return nullptr;
  return m_orderComposition.at(orderId);
}
std::vector<Trade> Participant::getHistoryOfTrades() const {
  return m_historyOfTrades;
}

std::size_t Participant::getNumberOfAssetsInPortfolio() const {
  return m_portfolio.size();
}

std::size_t Participant::getNumberOfOrdersPlaced() const {
  return m_placedOrders.size();
}

std::size_t Participant::getNumberOfTrades() const {
  return m_historyOfTrades.size();
}

void Participant::setParticipantID(const ParticipantID &newID) {
  m_participantID = newID;
}

bool Participant::isSymbolInPortfolio(const std::string &symbol) const {
  return (m_portfolio.contains(symbol));
}
