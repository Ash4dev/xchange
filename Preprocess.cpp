#include "Preprocess.h"
#include "utils/enums/OrderTypes.h"
#include <unordered_map>

std::unordered_map<OrderType::OrderType, int> PreProcessor::m_typeRank = {
    {OrderType::OrderType::Market, 1},
    {OrderType::OrderType::FillOrKill, 2},
    {OrderType::OrderType::ImmediateOrCancel, 3},
    {OrderType::OrderType::GoodForDay, 4},
    {OrderType::OrderType::GoodTillDate, 5},
    {OrderType::OrderType::AllOrNone, 6},
    {OrderType::OrderType::GoodTillCancel, 7},
};

void PreProcessor::QueueOrder(Order &order) { m_waitQueue.push(order); }

void PreProcessor::InsertOrder(Or)
