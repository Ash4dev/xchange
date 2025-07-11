#pragma once

#include "utils/alias/Fundamental.hpp"

struct OrderTraded {
  Symbol symbol;
  OrderID orderID;
  double price;
  Quantity quantityFilled;
  ParticipantID participantID;

  OrderTraded() = default;

  // need to make params type as const so that r-vals can be accepted too
  OrderTraded(const Symbol &symbol, const OrderID &orderID, const double price,
              const Quantity quantityFilled, const ParticipantID participantID)
      : symbol{symbol}, orderID{orderID}, price{price},
        quantityFilled{quantityFilled}, participantID{participantID} {
    ;
  }

  const Symbol &getSymbol() const noexcept { return symbol; }
  OrderID getOrderID() const noexcept { return orderID; }
  double getPrice() const noexcept { return price; }
  Quantity getQuantityFilled() const noexcept { return quantityFilled; }
  const ParticipantID &getParticipantID() const noexcept {
    return participantID;
  }
  ~OrderTraded() = default;
};
