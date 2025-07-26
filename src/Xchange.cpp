#include "include/Xchange.hpp"
#include "include/SymbolInfo.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderRel.hpp"
#include "utils/alias/ParticipantRel.hpp"
#include "utils/alias/PreProcessorRel.hpp"
#include "utils/alias/SymbolInfoRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

//////////////////////////////////////
////// Constructors of Xchange //////
////////////////////////////////////

std::unique_ptr<Xchange> Xchange::m_instance = nullptr;

Xchange::Xchange(std::size_t orderThreshold, std::chrono::milliseconds durationThreshold, const std::string &localTimeZone) : m_MAX_PENDING_ORDERS_THRESHOLD{orderThreshold}, m_MAX_PENDING_DURATION{durationThreshold}, localTimeZone{localTimeZone} {}

Xchange::Xchange(std::size_t orderThreshold,
                 std::chrono::milliseconds durationThreshold)
    : Xchange::Xchange(orderThreshold, durationThreshold, "Asia/Kolkata") {}

Xchange &Xchange::getInstance(int pendingThreshold, int pendingDuration, const std::string &localTimeZone)
{
  if (!m_instance && tradingHoursGMT.contains(localTimeZone))
  {
    m_instance.reset(
        new Xchange(static_cast<std::size_t>(pendingThreshold),
                    static_cast<std::chrono::milliseconds>(pendingDuration),
                    localTimeZone));
  }
  return *m_instance;
}

Xchange &Xchange::getInstance(int pendingThreshold, int pendingDuration)
{
  return Xchange::getInstance(pendingThreshold, pendingDuration, "Asia/Kolkata");
}

// no need to use static here
// since static here would indicate internal linkage and not it belongs to class
void Xchange::destroyInstance() { m_instance.reset(nullptr); }

//////////////////////////////////////
///////// PARTICIPANT FUNCTIONALITY /
////////////////////////////////////

ParticipantID Xchange::generateParticipantID(const std::string &govId)
{
  if (govID_partIDMap.count(govId) > 0)
    return govID_partIDMap.at(govId);
  std::size_t partCount = Xchange::getParticipantCount();
  ParticipantID partID = std::to_string(partCount);
  partID += ("_" + govId);
  return partID;
}

ParticipantID Xchange::addParticipant(const std::string &govID)
{
  if (m_govIDs.count(govID) > 0 && govID_partIDMap.count(govID) > 0)
    return govID_partIDMap.at(govID);
  m_govIDs.insert(govID);
  ParticipantID partID = Xchange::generateParticipantID(govID);
  ParticipantPointer freshParticipant = std::make_shared<Participant>();
  freshParticipant->setParticipantID(partID);
  m_participants[partID] = freshParticipant;
  govID_partIDMap[govID] = partID;
  return partID;
}

void Xchange::removeParticipant(const ParticipantID &participantID)
{
  if (m_participants.count(participantID) == 0)
    return;
  // lot to learn about smart pointers
  // removes ParticipantPointer from map (shared_ptr destructor called)
  // if ref count reaches zero, Participant is deleted
  m_participants.erase(participantID); // what about ParticipantPointer will it
                                       // be cleared immeadiately?
  std::string partGovID = "";
  for (auto it = participantID.rbegin(); it != participantID.rend(); it++)
  {
    char character = *it;
    if (character == '_')
      break;
    partGovID += character;
  }
  std::reverse(partGovID.begin(), partGovID.end());
  assert(m_govIDs.count(partGovID) > 0);
  m_govIDs.erase(partGovID);
  assert(govID_partIDMap.count(partGovID) > 0);
  govID_partIDMap.erase(partGovID);
}

// unified interface for placing order on the exchange
std::optional<OrderID> Xchange::placeOrder(
    const ParticipantID &participantID, const Actions::Actions action,
    const std::optional<OrderID> &oldOrderID,
    const std::optional<Symbol> &symbol, const std::optional<Side::Side> side,
    const std::optional<OrderType::OrderType> orderType,
    const std::optional<double> price, const std::optional<Quantity> quantity,
    const std::optional<std::string> &activationTime,
    const std::optional<std::string> &deactivationTime)
{

  if (m_participants.count(participantID) == 0)
    return std::nullopt; // participant must exist

  if (!symbol.has_value() || !orderType.has_value() || !side.has_value())
    return std::nullopt; // presence must (not modifiable: explicit c+a)

  if (action != Actions::Actions::Add && !oldOrderID.has_value())
    return std::nullopt; // cancel/modify must have oldOrderID for deletion

  OrderPointer orderptr{nullptr};
  bool canNOTplaceOrder =
      (!price.has_value() || !quantity.has_value() ||
       !activationTime.has_value() || !deactivationTime.has_value());

  // create the new order that will be created in this call
  if (!canNOTplaceOrder)
  {
    orderptr = m_participants[participantID]->recordNonCancelOrder(
        action, symbol.value(), orderType.value(), side.value(), price.value(),
        quantity.value(), participantID, activationTime.value(),
        deactivationTime.value());
  }

  SymbolInfoPointer symbolInfoPointer = m_symbolInfos.at(symbol.value());
  if (symbolInfoPointer == nullptr)
    return std::nullopt;

  PreProcessorPointer prePtr =
      ((side == Side::Side::Buy) ? symbolInfoPointer->m_bidprepro
                                 : symbolInfoPointer->m_askprepro);
  if (prePtr == nullptr)
    return std::nullopt;

  if (action == Actions::Actions::Add && orderptr != nullptr)
  {
    prePtr->InsertAddOrderIntoPreprocessing(orderptr);
    return orderptr->getOrderID();
  }

  auto const oldOrderInfo =
      m_participants.at(participantID)->getOrderInformation(oldOrderID.value());
  if (oldOrderInfo.side != side || oldOrderInfo.otype != orderType ||
      oldOrderInfo.symbol != symbol)
  {
    throw std::logic_error("can NOT alter side, ordertype, symbol while "
                           "modifying a previous order");
    return std::nullopt;
  }

  bool hasEntered =
      (prePtr->hasOrderEnteredOrderbook(oldOrderID.value(), orderType.value()));

  // getParticipantInfo(participantID)->recordCancelOrder(oldOrderID.value());
  if (!hasEntered)
    getParticipantInfo(participantID)->recordCancelOrder(oldOrderID.value());
  else
    std::cout << "WHY NOT FUCKING CANCEL RECORDED!" << std::endl;

  if (action == Actions::Actions::Modify)
  {
    if (orderptr == nullptr)
      return std::nullopt;

    prePtr->ModifyInPreprocessing(oldOrderID.value(), orderptr);
    return orderptr->getOrderID();
  }

  if (action == Actions::Actions::Cancel)
  {
    prePtr->RemoveFromPreprocessing(oldOrderID.value(), orderType.value());
    return std::nullopt;
  }
  return std::nullopt;
}

//////////////////////////////////////
///////// SYMBOL FUNCTIONALITY //////
////////////////////////////////////

void Xchange::tradeNewSymbol(const std::string &SYMBOL)
{
  if (m_symbolInfos.count(SYMBOL) > 0)
    return;
  SymbolInfoPointer symPtr{std::make_shared<SymbolInfo>(
      SYMBOL, m_MAX_PENDING_ORDERS_THRESHOLD, m_MAX_PENDING_DURATION, localTimeZone, Xchange::tradingHoursGMT.at(localTimeZone))};

  m_symbolInfos[SYMBOL] = symPtr;
}

void Xchange::retireOldSymbol(const std::string &SYMBOL)
{
  if (m_symbolInfos.count(SYMBOL) == 0)
    return;
  m_symbolInfos.erase(SYMBOL);
}

OrderBookPointer Xchange::getOrderBook(const Symbol &SYMBOL) const
{
  if (m_symbolInfos.count(SYMBOL) == 0)
    return nullptr;
  return m_symbolInfos.at(SYMBOL)->m_orderbook;
}

PreProcessorPointer Xchange::getPreProcessor(const Symbol &SYMBOL,
                                             const Side::Side &side) const
{
  if (m_symbolInfos.count(SYMBOL) == 0)
    return nullptr;
  if (side == Side::Side::Buy)
    return m_symbolInfos.at(SYMBOL)->m_bidprepro;
  else
    return m_symbolInfos.at(SYMBOL)->m_askprepro;
}

std::size_t Xchange::getOrderThreshold() const
{
  return m_MAX_PENDING_ORDERS_THRESHOLD;
}

std::uint64_t Xchange::getDurationThreshold() const
{
  return m_MAX_PENDING_DURATION.count();
}

const std::string &Xchange::getTimeZone() const
{
  return localTimeZone;
}

bool Xchange::isGovIDPresent(const std::string &govID) const
{
  return (m_govIDs.count(govID) > 0);
}

bool Xchange::canMapGovIDToParticipantID(const std::string &govID) const
{
  return (govID_partIDMap.count(govID) > 0);
}

ParticipantID
Xchange::getParticipantIDFromGovID(const std::string &govID) const
{
  if (govID_partIDMap.count(govID) == 0)
    return "";
  return govID_partIDMap.at(govID);
}

std::size_t Xchange::getParticipantCount() const
{
  return m_participants.size();
}

bool Xchange::isParticipantIDPresent(const std::string &partID) const
{
  return (m_participants.count(partID) > 0);
}

ParticipantPointer
Xchange::getParticipantInfo(const std::string &partID) const
{
  if (!isParticipantIDPresent(partID))
    return nullptr;
  return m_participants.at(partID);
}

std::size_t Xchange::getSymbolsTradedCount() const
{
  return m_symbolInfos.size();
}

bool Xchange::isSymbolTraded(const std::string &symbol) const
{
  return (m_symbolInfos.count(symbol) > 0);
}

// https://www.cmcmarkets.com/en-gb/trading-guides/stock-market-trading-hours
// https://en.wikipedia.org/wiki/List_of_tz_database_time_zones#TORONTO
const std::unordered_map<std::string, TimeTuple>
    Xchange::tradingHoursGMT = {
        {"Australia/Sydney", {std::chrono::hours(0) + std::chrono::minutes(0), std::chrono::hours(6) + std::chrono::minutes(0)}},     // ASX
        {"Asia/Kolkata", {std::chrono::hours(3) + std::chrono::minutes(45), std::chrono::hours(10) + std::chrono::minutes(0)}},       // BSE
        {"America/Sao_Paulo", {std::chrono::hours(13) + std::chrono::minutes(0), std::chrono::hours(20) + std::chrono::minutes(30)}}, // B3
        {"Europe/Paris", {std::chrono::hours(8) + std::chrono::minutes(0), std::chrono::hours(16) + std::chrono::minutes(30)}},       // Euronext
        {"Europe/Berlin", {std::chrono::hours(7) + std::chrono::minutes(0), std::chrono::hours(19) + std::chrono::minutes(0)}},       // FSX
        {"Asia/Hong_Kong", {std::chrono::hours(1) + std::chrono::minutes(30), std::chrono::hours(8) + std::chrono::minutes(0)}},      // HKEX
        {"Africa/Johannesburg", {std::chrono::hours(7) + std::chrono::minutes(0), std::chrono::hours(15) + std::chrono::minutes(0)}}, // JSE
        {"Asia/Seoul", {std::chrono::hours(0) + std::chrono::minutes(0), std::chrono::hours(6) + std::chrono::minutes(30)}},          // KRX
        {"Europe/London", {std::chrono::hours(8) + std::chrono::minutes(0), std::chrono::hours(16) + std::chrono::minutes(30)}},      // LSE
        {"America/New_York", {std::chrono::hours(14) + std::chrono::minutes(30), std::chrono::hours(21) + std::chrono::minutes(0)}},  // Nasdaq, NYSE
        {"Asia/Shanghai", {std::chrono::hours(1) + std::chrono::minutes(30), std::chrono::hours(7) + std::chrono::minutes(0)}},       // SSX, SZSE
        {"Europe/Zurich", {std::chrono::hours(8) + std::chrono::minutes(0), std::chrono::hours(16) + std::chrono::minutes(30)}},      // SIX
        {"Asia/Tokyo", {std::chrono::hours(0) + std::chrono::minutes(0), std::chrono::hours(6) + std::chrono::minutes(0)}},           // TSE
        {"America/Toronto", {std::chrono::hours(14) + std::chrono::minutes(30), std::chrono::hours(21) + std::chrono::minutes(0)}}    // TSX
};