#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/TradeStatus.hpp"
#include <cassert>
#include <include/Participant.hpp>

Participant::ParticipantOrderInfo::ParticipantOrderInfo(
    const OrderID &orderID, const Actions::Actions &action,
    const TradeStatus::TradeStatus &tradeStatus)
    : orderID{orderID}, action{action}, tradeStatus{tradeStatus} {
  ;
}

OrderPointer Participant::recordNonCancelOrder(
    const Actions::Actions action, const Symbol &symbol,
    const OrderType::OrderType orderType, const Side::Side side,
    const double price, const Quantity quantity,
    const ParticipantID &participantID, const std::string &activationTime,
    const std::string &deactivationTime) {

  assert(action != Actions::Actions::Cancel);
  OrderPointer orderptr =
      std::make_shared<Order>(symbol, orderType, side, price, quantity,
                              participantID, activationTime, deactivationTime);
  OrderID orderID = orderptr->getOrderID();
  ParticipantOrderInfo partorderinfo = Participant::ParticipantOrderInfo(
      orderID, action, TradeStatus::TradeStatus::Pending);
  m_placedOrders[orderID] = (partorderinfo);

  m_orderComposition[orderID] = orderptr;
  return orderptr;
}
