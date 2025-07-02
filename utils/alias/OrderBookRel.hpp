#include "include/OrderBook.hpp"
#include <memory>

// TODO: make shared for the moment, decide if unique (i think)
// only Xchange's single member has access to this
using OrderBookPointer = std::shared_ptr<OrderBook>;
