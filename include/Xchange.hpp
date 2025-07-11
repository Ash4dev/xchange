#pragma once

#include "include/SymbolInfo.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/ParticipantRel.hpp"
#include "utils/alias/SymbolInfoRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

// initial idea: make Xchange singleton
// benefits: only 1 instance, not customizable initialization
// drawbacks: global (static), not customizable initialization
// static is hell of a keyword (internal linkage, static duration, class prop)

class Xchange {
private:
  std::unordered_set<std::string> m_govIDs;
  std::unordered_map<ParticipantID, ParticipantPointer> m_participants;
  std::unordered_map<std::string, ParticipantID> govID_partIDMap;

  std::unordered_map<Symbol, SymbolInfoPointer> m_symbolInfos;

  std::size_t MAX_PENDING_ORDERS_THRESHOLD;
  std::chrono::milliseconds MAX_PENDING_DURATION;

  static std::unique_ptr<Xchange>
      m_instance; // must initialize outside class in main
  // private constructor
  Xchange(std::size_t pendingThreshold,
          std::chrono::milliseconds pendingDuration);

public:
  Xchange(const Xchange &) = delete;            // copy-constructor
  Xchange &operator=(const Xchange &) = delete; // copy-assignment

  static Xchange &getInstance(int pendingThreshold, int pendingDuration);
  static void destroyInstance();

  ParticipantID addParticipant(const std::string &govId);
  ParticipantID generateParticipantID(
      const std::string
          &govId); // part may wish to know their Id if they don't remember
  void removeParticipant(const ParticipantID &participantID);

  std::optional<OrderID> placeOrder(
      const ParticipantID &participantID, const Actions::Actions action,
      const std::optional<OrderID> &OrderID,
      const std::optional<Symbol> &symbol, const std::optional<Side::Side> side,
      const std::optional<OrderType::OrderType> orderType,
      const std::optional<double> price, const std::optional<Quantity> quantity,
      const std::optional<std::string> &activationTime,
      const std::optional<std::string> &deactivationTime);

  ParticipantPointer
  getParticipantInfo(const ParticipantID &participantID) const;
  std::size_t getParticipantCount() const;
  bool canMapGovIDToParticipantID(const std::string &govID) const;
  ParticipantID getParticipantIDFromGovID(const std::string &govId) const;

  void tradeNewSymbol(const Symbol &symbol);
  void retireOldSymbol(const Symbol &symbol);
  std::size_t getSymbolsTradedCount() const;
  OrderBookPointer getOrderBook(const Symbol &symbol) const;
  PreProcessorPointer getPreProcessor(const Symbol &symbol,
                                      const Side::Side &side) const;

  std::size_t getOrderThreshold() const;
  std::uint64_t getDurationThreshold() const;

  bool isGovIDPresent(const std::string &govId) const;
  bool isParticipantIDPresent(const std::string &partId) const;
  bool isSymbolTraded(const std::string &symbol) const;
};
