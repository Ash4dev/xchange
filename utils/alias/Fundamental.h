#pragma once

#include <chrono>
#include <cstdint>
#include <string>

// float32_t: c++23 introduction, 3 decimal place will be good
// multiplier of 1000 will ensure in ints (keep integral)
// add to constants file
using Price = std::int32_t;
using Quantity = std::uint64_t;
using Symbol = std::string;

// check for better alternatives: chrono
using TimeStamp = std::chrono::system_clock::time_point;
using OrderID = std::uint64_t;
