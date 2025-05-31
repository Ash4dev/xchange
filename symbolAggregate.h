#pragma once

#include "OrderBook.h"
#include "Preprocess.h"
#include <string>

class SymbolAggregate {
public:
  void updatePreprocessor();
  void updateOrderbook();

private:
  std::string m_symbol;
  PreProcessor m_preprocessor;
  OrderBook m_orderbook;
};
