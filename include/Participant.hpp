#pragma once

#include "Trade.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"
#include "utils/enums/TradeStatus.hpp"

#include <cstdint>
#include <string>
#include <vector>

class Participant {
private:
  struct ParticipantOrderInfo {
    OrderPointer m_orderPointer;
    TradeStatus::TradeStatus m_tradeStatus;
  };

  ParticipantID m_participantID;
  Portfolio m_portfolio;
  std::vector<ParticipantOrderInfo> m_placedOrders;
  std::vector<Trade> m_historyOfTrades;
  Amount m_remainingBalance;

public:
  // Constructors
  Participant(std::string &govID);

  static ParticipantID generateParticipantID(std::string &govID);
  void placeOrder(Actions::Actions action, const Symbol symbol,
                  const OrderType::OrderType orderType, const Side::Side side,
                  const double price, const Quantity quantity,
                  const std::string &activationTime = "",
                  const std::string &deactivationTime = "");

  void updatePortfolio(const std::vector<Trade> &recentTrades);
  void updateParticipantOrderInfo(const std::vector<Trade> &recentTrades);
  void addBalance(Amount extra);

  const std::uint64_t getParticipantCount();

  ParticipantID getParticipantID();
  Portfolio getPortfolio();
  std::vector<ParticipantOrderInfo> getPlacedOrders();
  std::vector<Trade> getHistoryOfTrades();
  Amount getRemainingBalance();
};
