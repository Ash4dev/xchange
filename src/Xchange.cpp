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
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>

//////////////////////////////////////
////// Constructors of Xchange //////
////////////////////////////////////

std::unique_ptr<Xchange> Xchange::m_instance = nullptr;

Xchange::Xchange(std::size_t pendingThreshold,
                 std::chrono::milliseconds pendingDuration)
    : MAX_PENDING_ORDERS_THRESHOLD{pendingThreshold},
      MAX_PENDING_DURATION{pendingDuration} {
  std::cout << "Xchange object initialized!" << std::endl;
};

Xchange &Xchange::getInstance(int pendingThreshold, int pendingDuration) {
  if (!m_instance) {
    m_instance.reset(
        new Xchange(static_cast<std::size_t>(pendingThreshold),
                    static_cast<std::chrono::milliseconds>(pendingDuration)));
  }
  return *m_instance;
}

// no need to use static here
// since static here would indicate internal linkage and not it belongs to class
void Xchange::destroyInstance() {
  m_instance.reset(nullptr);
  std::cout << "Xchange object destroyed!" << std::endl;
}

//////////////////////////////////////
///////// PARTICIPANT FUNCTIONALITY /
////////////////////////////////////

std::size_t Xchange::getParticipantCount() { return m_participants.size(); }

ParticipantID Xchange::generateParticipantID(const std::string &govId) {
  std::size_t partCount = Xchange::getParticipantCount();
  ParticipantID partID = std::to_string(partCount);
  partID += ("_" + govId);
  return partID;
}

void Xchange::addParticipant(const std::string &govID) {
  if (m_govIDs.count(govID) != 0)
    return;
  m_govIDs.insert(govID);
  ParticipantID partID = Xchange::generateParticipantID(govID);
  ParticipantPointer freshParticipant = std::make_shared<Participant>();
  freshParticipant->setParticipantID(partID);
  m_participants[partID] = freshParticipant;
}

void Xchange::removeParticipant(const ParticipantID &participantID) {
  if (m_participants.count(participantID) == 0)
    return;
  // lot to learn about smart pointers
  // removes ParticipantPointer from map (shared_ptr destructor called)
  // if ref count reaches zero, Participant is deleted
  m_participants.erase(participantID); // what about ParticipantPointer will it
                                       // be cleared immeadiately?
  std::string partGovID = "";
  for (auto it = participantID.rbegin(); it != participantID.rend(); it++) {
    char character = *it;
    if (character == '_')
      break;
    partGovID += character;
  }
  std::reverse(partGovID.begin(), partGovID.end());
  assert(m_govIDs.count(partGovID) > 0);
  m_govIDs.erase(partGovID);
}

// unified interface for placing order on the exchange
void Xchange::placeOrder(const ParticipantID &participantID,
                         const Actions::Actions action,
                         const std::optional<OrderID> &oldOrderID,
                         const std::optional<Symbol> &symbol,
                         const std::optional<Side::Side> side,
                         const std::optional<OrderType::OrderType> orderType,
                         const std::optional<double> price,
                         const std::optional<Quantity> quantity,
                         const std::optional<std::string> &activationTime,
                         const std::optional<std::string> &deactivationTime) {

  // Participant does not exist
  if (m_participants.count(participantID) == 0)
    return;

  // must be present in all orders (cancel, modify)
  // cannot modify symbol, orderType (if needed explicit cancel and add done)
  if (!symbol.has_value() || !orderType.has_value())
    return;

  OrderPointer orderptr{nullptr};
  bool canNOTplaceOrder =
      (!side.has_value() || !price.has_value() || !quantity.has_value() ||
       !activationTime.has_value() || !deactivationTime.has_value());

  if (!canNOTplaceOrder) {
    orderptr = m_participants[participantID]->recordNonCancelOrder(
        action, symbol.value(), orderType.value(), side.value(), price.value(),
        quantity.value(), participantID, activationTime.value(),
        deactivationTime.value());
  }

  SymbolInfoPointer symbolInfoPointer = m_symbolInfos[symbol.value()];

  if (action == Actions::Actions::Cancel ||
      action == Actions::Actions::Modify) {
    if (!oldOrderID.has_value())
      return;

    Side::Side decipheredSide =
        ((oldOrderID.value() & 0x1) ? (Side::Side::Buy) : (Side::Side::Sell));
    PreProcessorPointer prePtr =
        ((decipheredSide == Side::Side::Buy) ? symbolInfoPointer->m_bidprepro
                                             : symbolInfoPointer->m_askprepro);

    if (action == Actions::Actions::Modify && orderptr != nullptr)
      prePtr->ModifyInPreprocessing(oldOrderID.value(), orderptr);
    else
      prePtr->RemoveFromPreprocessing(oldOrderID.value(), orderType.value());
  }

  if (orderptr == nullptr)
    return;

  PreProcessorPointer prePtr = (side.value() == Side::Side::Buy)
                                   ? symbolInfoPointer->m_bidprepro
                                   : symbolInfoPointer->m_askprepro;
  prePtr->InsertAddOrderIntoPreprocessing(orderptr);
  // TODO: what about Participant?
}

//////////////////////////////////////
///////// SYMBOL FUNCTIONALITY //////
////////////////////////////////////

void Xchange::tradeNewSymbol(const std::string &SYMBOL) {
  if (m_symbolInfos.count(SYMBOL) > 0)
    return;
  SymbolInfoPointer symPtr{std::make_shared<SymbolInfo>(SYMBOL)};
  m_symbolInfos[SYMBOL] = symPtr;
}

void Xchange::retireOldSymbol(const std::string &SYMBOL) {
  if (m_symbolInfos.count(SYMBOL) == 0)
    return;
  m_symbolInfos.erase(SYMBOL);
}

OrderBookPointer Xchange::getOrderBook(const Symbol &SYMBOL) {
  if (m_symbolInfos.count(SYMBOL) == 0)
    return nullptr;
  return m_symbolInfos[SYMBOL]->m_orderbook;
}

PreProcessorPointer Xchange::getPreProcessor(const Symbol &SYMBOL,
                                             Side::Side &side) {
  if (m_symbolInfos.count(SYMBOL) == 0)
    return nullptr;
  if (side == Side::Side::Buy)
    return m_symbolInfos[SYMBOL]->m_bidprepro;
  else
    return m_symbolInfos[SYMBOL]->m_askprepro;
}
