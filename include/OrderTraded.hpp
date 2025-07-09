#pragma once

#include "utils/alias/Fundamental.hpp"

struct OrderTraded {
  OrderID orderID;
  double price;
  Quantity quantityFilled;
  ParticipantID participantID;

  OrderTraded() = default;

  // need to make params type as const so that r-vals can be accepted too
  OrderTraded(const OrderID &orderID, const double price,
              const Quantity quantityFilled, const ParticipantID participantID)
      : orderID{orderID}, price{price}, quantityFilled{quantityFilled},
        participantID{participantID} {
    ;
  }
  ~OrderTraded() = default;
};
