#pragma once

#include "OrderTraded.h"
#include "utils/alias/Fundamental.h"
#include <chrono>
#include <iomanip>
#include <iostream>

class Trade {
public:
  Trade(OrderTraded &bidOrder, OrderTraded &askOrder)
      : m_bidMatch{bidOrder}, m_askMatch{askOrder} {
    m_timeMatch = std::chrono::system_clock::now();
  }

  // https://stackoverflow.com/a/16449914
  const OrderTraded &getMatchedBid() const { return m_bidMatch; }
  const OrderTraded &getMatchedAsk() const { return m_askMatch; }
  const TimeStamp getMatchTime() const { return m_timeMatch; }

  const void printMatchTime() const {
    std::time_t tmatch = std::chrono::system_clock::to_time_t(m_timeMatch);
    std::tm *ptmatch = std::localtime(&tmatch); // local time conversion
    // not used to return string since put_time return value is undefined void?
    std::cout << std::put_time(ptmatch, "%Y-%m-%d %H:%M:%S") << std::endl;
  }

private:
  OrderTraded m_bidMatch;
  OrderTraded m_askMatch;
  TimeStamp m_timeMatch;
};
