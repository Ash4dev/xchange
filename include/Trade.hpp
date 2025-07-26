#pragma once

#include "include/OrderTraded.hpp"
#include "utils/alias/Fundamental.hpp"
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>

class Trade {
public:
  Trade(const OrderTraded &bidOrder, const OrderTraded &askOrder)
      : m_bidMatch{bidOrder}, m_askMatch{askOrder} {
    assert(bidOrder.getSymbol() == askOrder.getSymbol());
    m_symbol = bidOrder.getSymbol();
    m_timeMatch = getLocalTime();
  }

  // https://stackoverflow.com/a/16449914
  const Symbol getSymbol() const { return m_symbol; }
  const OrderTraded &getMatchedBid() const { return m_bidMatch; }
  const OrderTraded &getMatchedAsk() const { return m_askMatch; }
  const TimeStamp getMatchTime() const { return m_timeMatch; }

  // since returns void no sense of const
  void printMatchTime() const {
    std::time_t tmatch = std::chrono::system_clock::to_time_t(m_timeMatch);
    std::tm *ptmatch = std::localtime(&tmatch); // local time conversion
    // not used to return string since put_time return value is undefined void?
    std::cout << std::put_time(ptmatch, "%Y-%m-%d %H:%M:%S") << std::endl;
  }

  static TimeStamp getLocalTime() {
    auto now = std::chrono::system_clock::now();
    return now;
    // return now + std::chrono::hours(5) + std::chrono::minutes(30);
  }

private:
  Symbol m_symbol;
  OrderTraded m_bidMatch;
  OrderTraded m_askMatch;
  TimeStamp m_timeMatch;
};
