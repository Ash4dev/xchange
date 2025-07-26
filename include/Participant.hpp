#pragma once

#include "include/Trade.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"
#include <cstddef>
#include <unordered_map>
#include <vector>

class Participant
{
private:
  struct ParticipantOrderInfo
  {
    OrderID orderID;
    Actions::Actions action;
    Symbol symbol;
    OrderType::OrderType otype;
    Side::Side side;

    // rule of 3
    ParticipantOrderInfo() = default;
    ParticipantOrderInfo(const OrderID &orderID, const Actions::Actions &action,
                         const Symbol &symbol,
                         const OrderType::OrderType &otype,
                         const Side::Side &side);
    ~ParticipantOrderInfo() = default;
  };

  ParticipantID m_participantID;
  Portfolio m_portfolio;
  std::string localTimeZone;

  std::unordered_map<OrderID, OrderPointer> m_orderComposition;
  std::unordered_map<OrderID, ParticipantOrderInfo> m_placedOrders;

  std::vector<Trade> m_historyOfTrades;

public:
  Participant(const std::string &localTimeZoneOfParticipant);
  Participant();

  OrderPointer recordNonCancelOrder(const Actions::Actions action,
                                    const Symbol &symbol,
                                    const OrderType::OrderType orderType,
                                    const Side::Side side, const double price,
                                    const Quantity quantity,
                                    const ParticipantID &participantID,
                                    const std::string &activationTime = "",
                                    const std::string &deactivationTime = "");

  void recordCancelOrder(const OrderID &orderID);

  void recordTrades(const std::vector<Trade> &recentTrades);
  void updatePortfolio(const Side::Side &side, const Trade &trade);
  void updateOrderStatus(const OrderID &matchedID);

  std::string printParticipantLocalTime(const TimeStamp &tp);
  // const member functions can only be called by const participant objects
  ParticipantID getParticipantID() const;

  Portfolio getPortfolio() const;
  std::size_t getNumberOfAssetsInPortfolio() const;
  bool isSymbolInPortfolio(const std::string &symbol) const;
  double getValuationOfSymbol(const Symbol &symbol) const;
  double getValuationOfPortfolio() const;

  bool isParticularOrderPlacedByParticipant(const OrderID &orderID) const;
  ParticipantOrderInfo getOrderInformation(const OrderID &orderID) const;
  OrderPointer getOrder(const OrderID &OrderID) const;
  std::size_t getNumberOfOrdersPlaced() const;
  std::size_t getNumberOfPendingOrders() const;
  std::size_t getNumberOfOrdersBeingProcessed() const;
  std::size_t getNumberOfCancelledOrders() const;
  std::size_t getNumberOfProcessedOrders() const;

  static std::size_t lastProcessedTradeIndex;
  std::size_t getNumberOfTrades() const;
  std::vector<Trade> getHistoryOfTrades() const;

  void setParticipantID(const ParticipantID &newID);
};
