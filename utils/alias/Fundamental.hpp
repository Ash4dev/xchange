#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>

// float32_t: c++23 introduction, 2 decimal place will be good
// multiplier of 100 will ensure in ints (keep integral)
using Price = std::int32_t;
using Quantity = std::uint64_t;
using Symbol = std::string;

using TimeStamp = std::chrono::system_clock::time_point;
using OrderID = std::uint64_t;

using ParticipantID = std::string;
// net position can also be effectively negative (loss)
using Amount = std::int64_t;
using Portfolio = std::unordered_map<Symbol, Amount>;
