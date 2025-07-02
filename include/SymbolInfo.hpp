#pragma once

#include "utils/alias/Fundamental.hpp"
#include "utils/alias/OrderBookRel.hpp"
#include "utils/alias/PreProcessorRel.hpp"

#include <unordered_set>

// SymbolInfo essentially groups up data for a symbol
// no need to create a class for it

struct SymbolInfo {
  Symbol m_symbol;
  std::unordered_set<OrderBookPointer> m_orderbooks;
  std::unordered_set<PreProcessorPointer> m_bidprepro;
  std::unordered_set<PreProcessorPointer> m_askprepro;

  SymbolInfo(Symbol &symbol);
};
