#include "utils/alias/Fundamental.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/Side.hpp"

struct PreProcessorResult {
  Symbol symbol;
  Side::Side side;
  Actions::Actions action;
  Price price;
  Quantity quantity;
  ParticipantID participantID;
};

using OrderListSize = std::size_t;
struct OrderBookResult {
  Side::Side side;
  Symbol symbol;
  Price price;
  Quantity quantity;
  OrderListSize orderListSize;
};

struct TradeResult {
  Symbol symbol;
  Price price;
  Quantity quantity;
  ParticipantID buyerID;
  ParticipantID sellerID;
};
