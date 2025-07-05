#include "utils/alias/Fundamental.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"
#include <optional>

using UpdateCount = std::uint32_t;
struct Update {
  UpdateCount updateCount;
  Actions::Actions action;
  // following fields are optional for cancel, modify order
  std::optional<Symbol> symbol;
  std::optional<OrderType::OrderType> orderType;
  std::optional<Side::Side> side;
  std::optional<Price> price;
  std::optional<Quantity> quantity;
  std::optional<TimeStamp> activationTime;
  std::optional<TimeStamp> deactivationTime;
  std::optional<ParticipantID> participantID;
};
