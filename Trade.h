#pragma once

#include "OrderTraded.h"
#include "utils/alias/Fundamental.h"
#include <chrono>

class Trade {
public:
  Trade(OrderTraded &bidOrder, OrderTraded &askOrder)
      : m_bidMatch{bidOrder}, m_askMatch{askOrder} {
    m_timeMatch = std::chrono::system_clock::now().time_since_epoch().count();
  }

  // https://stackoverflow.com/a/16449914
  const OrderTraded &getMatchedBid() const { return m_bidMatch; }
  const OrderTraded &getMatchedAsk() const { return m_askMatch; }
  const TimeStamp getMatchTime() const { return m_timeMatch; }

private:
  OrderTraded m_bidMatch;
  OrderTraded m_askMatch;
  TimeStamp m_timeMatch;
};
