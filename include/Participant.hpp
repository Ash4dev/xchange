#pragma once

#include "include/Trade.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"
#include <unordered_map>
#include <vector>

class Participant {
private:
  struct ParticipantOrderInfo {
    OrderID orderID;
    Actions::Actions action;

    // rule of 3
    ParticipantOrderInfo() = default;
    ParticipantOrderInfo(const OrderID &orderID,
                         const Actions::Actions &action);
    ~ParticipantOrderInfo() = default;
  };

  ParticipantID m_participantID;
  Portfolio m_portfolio;

  std::unordered_map<OrderID, OrderPointer> m_orderComposition;
  std::unordered_map<OrderID, ParticipantOrderInfo> m_placedOrders;

  std::vector<Trade> m_historyOfTrades;

public:
  Participant() = default;

  OrderPointer recordNonCancelOrder(const Actions::Actions action,
                                    const Symbol &symbol,
                                    const OrderType::OrderType orderType,
                                    const Side::Side side, const double price,
                                    const Quantity quantity,
                                    const ParticipantID &participantID,
                                    const std::string &activationTime = "",
                                    const std::string &deactivationTime = "");

  void recordTrades(const std::vector<Trade> &recentTrades);
  void updatePortfolio(const Side::Side &side, const Trade &trade);
  void updateOrderStatus(const OrderID &matchedID);

  ParticipantID getParticipantID();
  Portfolio getPortfolio();
  ParticipantOrderInfo getOrderInformation(OrderID &orderID);
  std::vector<Trade> getHistoryOfTrades();

  void setParticipantID(const ParticipantID &newID);
};
