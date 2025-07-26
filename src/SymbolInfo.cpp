#include "include/SymbolInfo.hpp"
#include <chrono>
#include <memory>
#include <string>

SymbolInfo::SymbolInfo(const std::string &symbol) : m_symbol{symbol} {
  m_orderbook = std::make_shared<OrderBook>(symbol);
  m_bidprepro = std::make_shared<PreProcessor>(m_orderbook, true);
  m_askprepro = std::make_shared<PreProcessor>(m_orderbook, false);
}

SymbolInfo::SymbolInfo(const std::string &symbol,
                       const std::size_t &orderThresold,
                       const std::chrono::milliseconds &durationThreshold,
                      const std::string& localTimeZone,
                    const TimeTuple& timeTuple)
    : m_symbol{symbol} {
  m_orderbook = std::make_shared<OrderBook>(symbol);
  m_bidprepro = std::make_shared<PreProcessor>(m_orderbook, true, orderThresold,
                                               durationThreshold, localTimeZone, timeTuple);
  m_askprepro = std::make_shared<PreProcessor>(
      m_orderbook, false, orderThresold, durationThreshold, localTimeZone, timeTuple);
}
