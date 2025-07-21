#pragma once

#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderBookRel.hpp"
#include "utils/alias/PreProcessorRel.hpp"
#include <chrono>
#include <cstddef>

// SymbolInfo essentially groups up data for a symbol
// no need to create a class for it

struct SymbolInfo {
  Symbol m_symbol;
  OrderBookPointer m_orderbook;
  PreProcessorPointer m_bidprepro;
  PreProcessorPointer m_askprepro;

  SymbolInfo(const Symbol &symbol);
  SymbolInfo(const Symbol &symbol, const std::size_t &orderThreshold,
             const std::chrono::milliseconds &durationThreshold);
};
