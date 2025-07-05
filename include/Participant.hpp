#pragma once

#include "include/Trade.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"
#include "utils/enums/TradeStatus.hpp"
#include <unordered_map>
#include <vector>

class Participant {
private:
  struct ParticipantOrderInfo {
    OrderID orderID;
    Actions::Actions action;
    TradeStatus::TradeStatus tradeStatus;

    ParticipantOrderInfo(const OrderID &orderID, const Actions::Actions &action,
                         const TradeStatus::TradeStatus &tradeStatus);
  };

  ParticipantID m_participantID;
  Portfolio m_portfolio;

  std::unordered_map<OrderID, OrderPointer> m_orderComposition;
  std::unordered_map<OrderID, ParticipantOrderInfo> m_placedOrders;

  std::vector<Trade> m_historyOfTrades;

public:
  // Constructors
  Participant() = default;

  OrderPointer recordNonCancelOrder(const Actions::Actions action,
                                    const Symbol &symbol,
                                    const OrderType::OrderType orderType,
                                    const Side::Side side, const double price,
                                    const Quantity quantity,
                                    const ParticipantID &participantID,
                                    const std::string &activationTime = "",
                                    const std::string &deactivationTime = "");

  void updatePortfolio(const std::vector<Trade> &recentTrades);
  void updateParticipantOrderInfo(const std::vector<Trade> &recentTrades);

  void setParticipantID(ParticipantID &newId);
  ParticipantID getParticipantID();
  Portfolio getPortfolio();
  std::vector<ParticipantOrderInfo> getPlacedOrders();
  std::vector<Trade> getHistoryOfTrades();
};
