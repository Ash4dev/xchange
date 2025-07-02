#pragma once

#include "utils/alias/Fundamental.hpp"
#include "utils/alias/SymbolInfoRel.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

struct PreProcessorArguments {
  std::size_t MAX_PENDING_ORDER_THRESHOLD;
  std::chrono::milliseconds MAX_PENDING_DURATION_THRESHOLD;
};

using UpdateCount = std::uint32_t;
struct Update {
  UpdateCount updateCount;
  Actions::Actions action;
  // following fields are optional for cancel, modify order
  std::optional<Symbol> symbol;
  std::optional<OrderType::OrderType> orderType;
  std::optional<Side::Side> side;
  std::optional<Price> price;
  std::optional<Quantity> quantity;
  std::optional<TimeStamp> activationTime;
  std::optional<TimeStamp> deactivationTime;
  std::optional<ParticipantID> participantID;
};

struct PreProcessorResult {
  Symbol symbol;
  Side::Side side;
  Actions::Actions action;
  Price price;
  Quantity quantity;
  ParticipantID participantID;
};

using OrderListSize = std::size_t;
struct OrderBookResult {
  Side::Side side;
  Symbol symbol;
  Price price;
  Quantity quantity;
  OrderListSize orderListSize;
};

struct TradeResult {
  Symbol symbol;
  Price price;
  Quantity quantity;
  ParticipantID buyerID;
  ParticipantID sellerID;
};

class TestHandler {
private:
  std::size_t m_MAX_PENDING_ORDER_THRESHOLD;
  std::chrono::milliseconds m_MAX_PENDING_DURATION_THRESHOLD;
  void parsePreprocessorArguments();

  std::vector<Update> m_finalUpdates;
  void parseAddUpdate();
  void parseModifyUpdate();
  void parseCancelUpdate();
  void parseUpdate(std::ifstream &testFile);
  void parseUpdates();
  void performUpdate();

  std::vector<PreProcessorResult> m_preResults;
  std::vector<OrderBookResult> m_obResults;
  std::vector<TradeResult> m_trResults;
  void parsePreprocessorResults(std::ifstream &testFile);
  void parseOrderbookResults(std::ifstream &testFile);
  void parseTradeResults(std::ifstream &testFile);
  void parseResult();
  void parseResults();

public:
  TestHandler() = default;
  void parseTestFile(const std::string &testFilePath);
  void performUpdates();
  bool verifyResults(const SymbolInfoPointer &symbolInfo);

  std::size_t getPreProcessorArgs();
  std::vector<Update> getUpdates();
  std::vector<PreProcessorResult> getPreProcessorResults();
  std::vector<OrderBookResult> getOrderBookResults();
  std::vector<TradeResult> getTradesResults();
};
