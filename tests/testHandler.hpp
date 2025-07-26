#pragma once

#include "utils/alias/ResultRel.hpp"
#include "utils/alias/SymbolInfoRel.hpp"
#include "utils/alias/UpdateRel.hpp"

#include <chrono>
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

struct PreProcessorArguments
{
  std::size_t MAX_PENDING_ORDER_THRESHOLD;
  std::chrono::milliseconds m_MAX_PENDING_DURATION_THRESHOLD;
};

class TestHandler
{
private:
  std::size_t m_MAX_PENDING_ORDER_THRESHOLD;
  std::chrono::milliseconds m_m_MAX_PENDING_DURATION_THRESHOLD;
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
