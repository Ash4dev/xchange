#include "include/SymbolInfo.hpp"
#include <memory>
#include <string>

SymbolInfo::SymbolInfo(const std::string &symbol) : m_symbol{symbol} {
  m_orderbook = std::make_shared<OrderBook>(symbol);
  m_bidprepro = std::make_shared<PreProcessor>(m_orderbook, true);
  m_askprepro = std::make_shared<PreProcessor>(m_orderbook, false);
}
