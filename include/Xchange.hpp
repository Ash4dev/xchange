#pragma once

#include "include/SymbolInfo.hpp"
#include "utils/alias/Fundamental.hpp"
#include "utils/alias/ParticipantRel.hpp"
#include "utils/alias/SymbolInfoRel.hpp"

#include <chrono>
#include <string>
#include <unordered_map>
#include <unordered_set>

// only 1 instance of Xchange should ever exist (logger/file system)
// singleton pattern (https://www.youtube.com/watch?v=eLAvry56vLU)
// Mike shah version: https://onlinegdb.com/P4hksDfNw
// multiple design patterns exist (learn later)

class Xchange {
private:
  std::unordered_set<std::string> m_govIDs;
  std::unordered_map<ParticipantID, ParticipantPointer> m_participants;

  std::unordered_map<Symbol, SymbolInfoPointer> m_symbolInfos;

  std::size_t MAX_PENDING_ORDERS_THRESHOLD;
  std::chrono::milliseconds MAX_PENDING_DURATION;

  // constructor
  Xchange(std::size_t pendingThreshold,
          std::chrono::milliseconds pendingDuration);

public:
  void addParticipant(const std::string &govId);
  void removeParticipant();
  ParticipantPointer getParticipantInfo(const ParticipantID &participantID);

  void tradeNewSymbol(const Symbol &symbol);
  void retireOldSymbol(const Symbol &symbol);
  OrderBookPointer getOrderBook();
  PreProcessorPointer getPreProcessor(Side::Side &side);
};
