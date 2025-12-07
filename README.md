# Xchange - Toy Order Matching Engine

## Note before you begin:
- This is my first ever software project. I literally had no idea about software development and C++ in particular. I admit there is a lot of scope for improvement. 
- I have learned a lot more stuff along the way(OOPS, C++). I plan to improve this project further. Kindly suggest ways for the same if something pops up in your head.

### DISCLAIMER: The README.md for this project has been generated using Copilot Agent. However, all of the code is based on me trying to figure out stuff on the fly.

A wanna-be sophisticated C++ implementation of a financial exchange system featuring real-time order matching, comprehensive order type support, and  order preprocessing. Built with modern C++17 features for attempt at optimal performance and type safety.

## 🎯 Project Overview

**Xchange** is a complete order matching engine that simulates real-world financial exchange operations. It handles multiple participants trading various symbols with support for 10 different order types, time-based order activation, and priority-based matching algorithms.

### Key Features
- **Multi-Symbol Trading**: Support for trading multiple financial instruments simultaneously
- **10 Order Types**: Comprehensive support for various order types including Market, Limit, GoodTillCancel, GoodTillDate, GoodForDay, GoodAfterTime, MarketOnOpen, MarketOnClose, ImmediateOrCancel, FillOrKill, and AllOrNone
- **Smart Order Preprocessing**:  order queue management with configurable thresholds
- **Priority-Based Matching**: Price-time priority for fair and efficient order execution
- **Participant Management**: Track multiple participants with portfolios and trade history
- **Time Zone Support**: Configurable time zones with trading hours validation
- **Holiday Calendar**: Market holiday awareness for realistic trading simulations

---

## 🏗️ Architecture

### System Components

The system is organized into a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                         Xchange                              │
│  (Central Exchange - Singleton Pattern)                      │
│  • Participant Management                                     │
│  • Symbol Registration                                        │
│  • Order Routing                                             │
└─────────────────┬───────────────────────────┬───────────────┘
                  │                           │
        ┌─────────▼─────────┐       ┌────────▼────────────┐
        │   Participant     │       │   SymbolInfo        │
        │   • Portfolio     │       │   • OrderBook       │
        │   • Orders        │       │   • PreProcessors   │
        │   • Trade History │       │     (Buy & Sell)    │
        └───────────────────┘       └──────────┬──────────┘
                                               │
                    ┌──────────────────────────┴────────────┐
                    │                                       │
         ┌──────────▼──────────┐                ┌─────────▼────────┐
         │   PreProcessor      │                │   OrderBook      │
         │   (Per Side/Symbol) │                │   (Per Symbol)   │
         │   • Order Queue     │◄───────────────┤   • Bid Levels   │
         │   • Time Validation │                │   • Ask Levels   │
         │   • Type Ranking    │                │   • Matching     │
         └──────────┬──────────┘                └──────────┬───────┘
                    │                                      │
                    │                                      │
         ┌──────────▼──────────┐                ┌─────────▼────────┐
         │   Order             │                │   Level          │
         │   • Order Details   │◄───────────────│   • Price Level  │
         │   • Time Attributes │                │   • Order Queue  │
         │   • Status          │                │   • Quantity     │
         └─────────────────────┘                └──────────┬───────┘
                                                           │
                                                  ┌────────▼────────┐
                                                  │   Trade         │
                                                  │   • Matched     │
                                                  │     Orders      │
                                                  └─────────────────┘
```

---

## 📦 Component Details

### 1. **Xchange** (Central Exchange)
**File**: `include/Xchange.hpp`, `src/Xchange.cpp`

The main orchestrator implementing the **Singleton Pattern** to ensure a single exchange instance.

**Responsibilities**:
- Manage participants (add, remove, lookup by government ID)
- Register and retire tradable symbols
- Route orders to appropriate order books and preprocessors
- Validate trading hours and market status
- Provide system-wide configuration (order thresholds, pending duration)

**Key Features**:
- Singleton pattern with custom initialization parameters
- Maps government IDs to unique participant IDs
- Manages `SymbolInfo` objects for each tradable symbol
- Configurable pending order thresholds and durations
- Time zone aware with trading hours validation

### 2. **SymbolInfo** (Symbol Container)
**File**: `include/SymbolInfo.hpp`, `src/SymbolInfo.cpp`

A lightweight struct that groups all components needed for trading a specific symbol.

**Components**:
- One `OrderBook` per symbol
- Two `PreProcessor` instances (one for bids, one for asks)
- Symbol identifier

**Purpose**: Encapsulates symbol-specific trading infrastructure for clean organization.

### 3. **OrderBook** (Matching Engine)
**File**: `include/OrderBook.hpp`, `src/OrderBook.cpp`

The core matching engine that maintains price levels and executes trades.

**Data Structure**:
- **Bid Levels**: `std::map<Price, Level, std::greater<Price>>` (highest price first)
- **Ask Levels**: `std::map<Price, Level, std::less<Price>>` (lowest price first)
- **Trade History**: Vector of executed trades

**Operations**:
- `AddOrder()`: Insert order at appropriate price level
- `CancelOrder()`: Remove order from order book
- `ModifyOrder()`: Update existing order
- `MatchPotentialOrders()`: Execute price-time priority matching

**Matching Algorithm**:
1. Check if bid price ≥ ask price (crossing condition)
2. Match orders at the crossing price
3. Fill orders based on time priority (FIFO at each price level)
4. Generate `Trade` objects for matched orders
5. Update quantities and remove fully filled orders

### 4. **PreProcessor** (Order Queue Manager)
**File**: `include/Preprocess.hpp`, `src/PreProcess.cpp`

 order queue that validates and schedules orders before they enter the OrderBook.

**Key Responsibilities**:
- Buffer orders and validate activation/deactivation times
- Rank orders by type priority (Market > ImmediateOrCancel > GoodTillCancel, etc.)
- Enforce pending order thresholds to prevent queue overflow
- Check trading hours and holiday calendar
- Flush qualified orders to OrderBook based on time and count triggers

**Order Type Ranking** (Higher rank = Higher priority):
```
Market (highest priority)
FillOrKill
ImmediateOrCancel
MarketOnClose
MarketOnOpen
GoodAfterTime
GoodForDay
GoodTillDate
GoodTillCancel
AllOrNone (lowest priority)
```

**Configuration**:
- `MAX_PENDING_ORDERS_THRESHOLD`: Trigger flush when queue exceeds this count
- `MAX_PENDING_DURATION`: Flush interval in milliseconds
- Holiday calendar with Indian market holidays for 2025
- Trading hours validation (configurable per symbol)

### 5. **Level** (Price Level)
**File**: `include/Level.hpp`, `src/Level.cpp`

Represents a single price point in the order book with multiple orders at that price.

**Data Structure**:
- `m_orderList`: Linked list of orders (maintains time priority)
- `m_info`: Hash map for O(1) order lookup
- `m_quantity`: Aggregate quantity at this level

**Operations**:
- `AddOrder()`: Append to end of order list (time priority)
- `CancelOrder()`: Remove specific order
- `ModifyOrder()`: Update order details
- `removeMatchedOrder()`: Clean up after trade execution

**Benefits**:
- Fast aggregation of quantities at each price
- Efficient order lookup with O(1) complexity
- Maintains strict time priority within price level

### 6. **Order** (Order Object)
**File**: `include/Order.hpp`, `src/Order.cpp`

Core data structure representing a single order.

**Attributes**:
- Symbol, side (Buy/Sell), price, quantity
- Order type (Market, Limit, GoodTillDate, etc.)
- Participant ID
- Timestamps: creation, activation, deactivation
- Order status (NotProcessed, Processing, Fulfilled, Cancelled)
- Unique Order ID (encoded with timestamp, price, and side)

**Key Methods**:
- `encodeOrderID()`: Generate unique 64-bit order ID
- `FillPartially()`: Update quantity on partial fill
- `convertDateTimeToTimeStamp()`: Parse time strings
- `returnReadableTime()`: Format timestamps for display

### 7. **Participant** (Trader)
**File**: `include/Participant.hpp`, `src/Participant.cpp`

Represents a market participant with portfolio and order history.

**Tracks**:
- Portfolio: Map of symbols to net positions (amounts)
- Order composition: All orders placed by participant
- Trade history: Record of all executed trades
- Order status: Pending, processing, fulfilled, cancelled counts

**Capabilities**:
- Record new orders (non-cancel and cancel)
- Update portfolio on trade execution
- Calculate portfolio valuation
- Query order and trade statistics

### 8. **Trade** (Matched Order Pair)
**File**: `include/Trade.hpp`

Immutable record of a successful trade between two orders.

**Contains**:
- Matched bid order (buyer)
- Matched ask order (seller)
- Symbol
- Match timestamp

**Purpose**: Provides audit trail and trade history for participants and the exchange.

---

## 🔗 Component Interactions

### Order Flow (Complete Lifecycle)

```
1. Participant Submits Order
   ↓
2. Xchange.placeOrder()
   • Validates participant exists
   • Routes to appropriate SymbolInfo
   ↓
3. PreProcessor.InsertIntoPreprocessing()
   • Validates time attributes
   • Checks market hours & holidays
   • Adds to type-ranked queue
   ↓
4. PreProcessor.TryFlush() [Triggered by time/count]
   • Sorts orders by type priority
   • Validates activation times
   • Flushes qualified orders
   ↓
5. OrderBook.AddOrder()
   • Finds/creates price level
   • Adds order to level
   ↓
6. OrderBook.MatchPotentialOrders()
   • Checks for crossing orders
   • Executes trades (price-time priority)
   • Generates Trade objects
   ↓
7. Participant.recordTrades()
   • Updates portfolio
   • Records trade history
   • Updates order status
```

### Data Flow Example

```
Participant A: Buy 100 AAPL @ $150
Participant B: Sell 50 AAPL @ $149

Flow:
1. Both orders enter respective PreProcessors (Buy/Sell)
2. PreProcessor flushes orders to OrderBook
3. OrderBook detects crossing ($150 bid ≥ $149 ask)
4. Match 50 shares at $149 (ask price)
5. Create Trade object
6. Participant A: +50 AAPL, -$7,450
7. Participant B: -50 AAPL, +$7,450
8. Participant A's order: 50 shares remaining (partial fill)
9. Participant B's order: fully filled, removed from book
```

---

## 🎨 Key C++ Features Utilized

### Modern C++ Features
1. **`#pragma once`**: Modern header guard replacing traditional `#ifndef` guards
2. **`std::chrono`**: Comprehensive time handling with `system_clock::time_point` (C++11)
3. **`constexpr`**: Compile-time holiday array in PreProcessor (C++11)
4. **Range-based loops**: Used throughout for container iteration (C++11)
5. **C++23 Standard**: Project is compiled with `-std=c++23` flag for latest language features

### Smart Pointers & Memory Management
```cpp
// Shared ownership across multiple preprocessors
std::shared_ptr<OrderBook> m_orderbookPtr;

// Unique ownership in Xchange singleton
static std::unique_ptr<Xchange> m_instance;

// Type aliases for clarity
using OrderPointer = std::shared_ptr<Order>;
using LevelPointer = std::shared_ptr<Level>;
using PreProcessorPointer = std::shared_ptr<PreProcessor>;
```

**Benefits**:
- Automatic memory management (no manual `delete`)
- Shared ownership for OrderBook across preprocessors
- Exception-safe resource management

### STL Containers (Strategic Choices)

#### 1. `std::map` (OrderBook Levels)
```cpp
std::map<Price, LevelPointer, std::greater<Price>> m_bids;  // Max-heap behavior
std::map<Price, LevelPointer, std::less<Price>> m_asks;     // Min-heap behavior
```
**Reason**: Maintains sorted price levels for efficient matching

#### 2. `std::unordered_map` (Fast Lookups)
```cpp
std::unordered_map<OrderID, OrderPointer> m_orderComposition;
std::unordered_map<ParticipantID, ParticipantPointer> m_participants;
```
**Reason**: O(1) lookup for orders and participants by ID

#### 3. `std::set` (Ordered Collections)
```cpp
std::set<OrderActionInfo> m_laterProcessOrders;
```
**Reason**: Automatic sorting with custom comparator, no duplicates

#### 4. `std::list` (Order Queue at Level)
```cpp
OrderList m_orderList;  // typedef std::list<OrderPointer>
```
**Reason**: Efficient insertion/deletion while maintaining time priority

#### 5. `std::vector` (Trade History)
```cpp
std::vector<Trade> m_trades;
```
**Reason**: Fast sequential access for trade records

### Enumerations (Type Safety)
```cpp
namespace Side {
    enum Side { Buy, Sell };
}

namespace OrderType {
    enum OrderType { 
        AllOrNone, GoodTillCancel, GoodTillDate, 
        GoodForDay, GoodAfterTime, MarketOnOpen,
        MarketOnClose, ImmediateOrCancel, 
        FillOrKill, Market 
    };
}

namespace OrderStatus {
    enum OrderStatus { 
        NotProcessed, Processing, Fulfilled, Cancelled 
    };
}
```
**Benefits**: Type-safe constants with namespace isolation

### Templates & Type Aliases
```cpp
// Type aliases for readability
using Price = std::int32_t;
using Quantity = std::uint64_t;
using Symbol = std::string;
using TimeStamp = std::chrono::system_clock::time_point;
using OrderID = std::uint64_t;
using ParticipantID = std::string;
using Amount = double;
using Portfolio = std::unordered_map<Symbol, Amount>;

// Template comparators in std::map
std::map<Price, Level, std::greater<Price>> bids;  // Descending
std::map<Price, Level, std::less<Price>> asks;     // Ascending
```

### Design Patterns

#### 1. **Singleton Pattern** (Xchange)
```cpp
class Xchange {
private:
    static std::unique_ptr<Xchange> m_instance;
    Xchange(...);  // Private constructor
    
public:
    Xchange(const Xchange&) = delete;             // Delete copy
    Xchange& operator=(const Xchange&) = delete;  // Delete assignment
    
    static Xchange& getInstance(...);
    static void destroyInstance();
};
```
**Purpose**: Ensure single exchange instance across the system

#### 2. **RAII** (Resource Acquisition Is Initialization)
- Smart pointers automatically manage Order, Level, OrderBook lifetimes
- No manual memory management required
- Exception-safe cleanup

#### 3. **Struct for Data Aggregation** (SymbolInfo)
```cpp
struct SymbolInfo {
    Symbol m_symbol;
    OrderBookPointer m_orderbook;
    PreProcessorPointer m_bidprepro;
    PreProcessorPointer m_askprepro;
};
```
**Purpose**: Lightweight container for related components

### Operator Overloading
```cpp
bool Order::operator<(const Order& other) const {
    return m_timestamp < other.m_timestamp;  // Time priority
}

bool Order::operator==(const Order& other) const {
    return m_orderID == other.m_orderID;
}
```
**Purpose**: Enable natural comparison syntax and STL container compatibility

### Static Members & Methods
```cpp
class OrderBook {
public:
    static Price decodePriceFromOrderID(const OrderID orderID);
    static Side::Side decodeSideFromOrderID(const OrderID orderID);
};

class PreProcessor {
private:
    static std::unordered_map<OrderType::OrderType, int> m_typeRank;
    static constexpr std::array<std::tuple<...>, 15> m_holidays = {...};
};
```
**Purpose**: Shared state/logic across instances, utility functions

### Optional Type (Safety)
```cpp
std::optional<Trade> AddOrder(Order& order);
std::optional<OrderID> placeOrder(...);
std::optional<OrderActionInfo> getOrderInfo(const OrderID& orderID);
```
**Benefits**: 
- Explicit handling of "no value" cases
- Avoid null pointer errors
- Clear API contracts

### Const Correctness
```cpp
Price getPrice() const { return m_price; }
Symbol getSymbol() const { return m_symbol; }
const OrderTraded& getMatchedBid() const { return m_bidMatch; }
```
**Benefits**: 
- Compile-time guarantees that methods don't modify state
- Enables optimization
- Prevents accidental mutations

### Inline Functions & Getters
```cpp
Symbol getSymbol() const { return m_symbol; }
Price getPrice() const { return m_price; }
```
**Benefits**: Compiler can inline for zero-cost abstraction

---

## 📁 Project Structure

```
xchange/
├── include/              # Public header files
│   ├── Xchange.hpp      # Central exchange
│   ├── OrderBook.hpp    # Matching engine
│   ├── Preprocess.hpp   # Order preprocessing
│   ├── Participant.hpp  # Trader representation
│   ├── Level.hpp        # Price level
│   ├── Order.hpp        # Order object
│   ├── Trade.hpp        # Trade record
│   ├── OrderTraded.hpp  # Matched order details
│   └── SymbolInfo.hpp   # Symbol container
├── src/                 # Implementation files
│   ├── Xchange.cpp
│   ├── OrderBook.cpp
│   ├── PreProcess.cpp
│   ├── Participant.cpp
│   ├── Level.cpp
│   ├── Order.cpp
│   └── SymbolInfo.cpp
├── utils/               # Utilities and type definitions
│   ├── enums/          # Enumerations
│   │   ├── Side.hpp           # Buy/Sell
│   │   ├── OrderTypes.hpp     # 10 order types
│   │   ├── OrderStatus.hpp    # Order lifecycle states
│   │   └── Actions.hpp        # Add/Modify/Cancel
│   ├── alias/          # Type aliases
│   │   ├── Fundamental.hpp    # Basic types (Price, Quantity, etc.)
│   │   ├── OrderRel.hpp       # Order-related pointers
│   │   ├── LevelRel.hpp       # Level-related pointers
│   │   └── ...
│   ├── helpers/        # Helper functions
│   │   ├── HelperFunctions.hpp
│   │   └── HelperFunctions.cpp
│   └── Constants.hpp   # System constants
├── tests/              # Test files (GTest framework)
│   ├── core-xchange.tests.cpp    # Xchange tests
│   ├── core-part.tests.cpp       # Participant tests
│   ├── core-pre.tests.cpp        # PreProcessor tests
│   ├── actual/
│   │   ├── core-ob.tests.cpp     # OrderBook tests
│   │   └── core-lev.tests.cpp    # Level tests
│   ├── cases/          # Test case data files
│   └── testHandler.cpp # Test utilities
├── makefile            # Build configuration
├── .gitignore
└── README.md           # This file
```

---

## 🛠️ Building and Testing

### Prerequisites
- **Compiler**: g++ with C++23 support (GCC 12+ or Clang 15+ recommended)
- **Build System**: GNU Make
- **Testing**: Google Test (GTest) framework

### Build Commands

```bash
# Clean build artifacts
make clean

# Build object files
make getObjectFiles

# Build and run (uses tests/core-pre.cpp as entry point)
make fresh

# Run existing executable
make run
```

### Testing Commands

```bash
# Run test health check
make testHealth

# Build all test binaries
make testFiles

# Run all tests
make test

# Run all tests and clean up
make testAll

# Clean test artifacts
make cleanTests
```

### Build Configuration

The project uses a sophisticated Makefile with:
- **Compiler**: `g++`
- **C++ Standard**: `-std=c++23`
- **Flags**: `-Wall -Wextra -pedantic-errors` (strict warnings)
- **Debug**: `-ggdb -O0` (debug symbols, no optimization)
- **Include Path**: `-I.` (root directory)

**Directory Structure**:
- `src/` → `obj/*.o` (compiled objects)
- `tests/*.tests.cpp` → `tests/bin/*` (test executables)

---

## 🧪 Test Coverage

The project includes comprehensive tests organized by component:

### Test Files
1. **core-xchange.tests.cpp**: Exchange operations (participants, symbols, order routing)
2. **core-part.tests.cpp**: Participant portfolio and order tracking
3. **core-pre.tests.cpp**: PreProcessor queue management and order validation
4. **core-ob.tests.cpp**: OrderBook matching logic
5. **core-lev.tests.cpp**: Level operations

### Test Cases
The `tests/cases/` directory contains structured test data for:
- **PreProcessor**: Order insertion, removal, modification, queue flushing
- **OrderBook**: Matching scenarios (no match, partial match, complete match, time priority)

---

## 🚀 Usage Example

```cpp
// Initialize exchange with thresholds
Xchange& xchange = Xchange::getInstance(
    50,                      // Pending order threshold
    5000,                    // Pending duration (ms)
    "America/New_York"       // Time zone
);

// Add participants
ParticipantID trader1 = xchange.addParticipant("GOV_ID_12345");
ParticipantID trader2 = xchange.addParticipant("GOV_ID_67890");

// Enable symbol trading
xchange.tradeNewSymbol("AAPL");

// Place buy order
auto orderID = xchange.placeOrder(
    trader1,                     // Participant
    Actions::Add,                // Action
    std::nullopt,                // Order ID (null for new)
    "AAPL",                      // Symbol
    Side::Buy,                   // Side
    OrderType::GoodTillCancel,   // Order type
    15000,                       // Price ($150.00)
    100,                         // Quantity
    std::nullopt,                // Activation time
    std::nullopt                 // Deactivation time
);

// Place sell order
xchange.placeOrder(
    trader2,
    Actions::Add,
    std::nullopt,
    "AAPL",
    Side::Sell,
    OrderType::Market,
    14900,                       // Price ($149.00)
    50,                          // Quantity
    std::nullopt,
    std::nullopt
);

// Check trades
auto trades = xchange.getTradesExecuted("AAPL");
for (const auto& trade : trades) {
    std::cout << "Trade executed: " << trade.getSymbol() << std::endl;
}

// Cleanup
Xchange::destroyInstance();
```

---

## 🎯 Key Highlights

### Performance Optimizations
1. **O(1) Order Lookup**: Hash maps for instant order retrieval
2. **Sorted Price Levels**: `std::map` with custom comparators for efficient matching
3. **Time Priority**: Linked lists maintain FIFO order at each price level
4. **Smart Pointer Sharing**: Avoid deep copies with shared ownership

### Scalability Features
1. **Multi-Symbol Support**: Independent order books per symbol
2. **Configurable Thresholds**: Adjust queue sizes and flush intervals
3. **Extensible Order Types**: Easy to add new order type behaviors
4. **Modular Architecture**: Components can be tested and optimized independently

### Real-World Considerations
1. **Time Zone Awareness**: Convert between local and GMT times
2. **Trading Hours Validation**: Prevent orders outside market hours
3. **Holiday Calendar**: Skip non-trading days
4. **Order Status Tracking**: Monitor order lifecycle from submission to fulfillment

---

## 📚 Learning Resources

### C++ Features Used
- **Modern C++**: [C++23 Standard](https://en.cppreference.com/w/cpp/23)
- **Smart Pointers**: [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr), [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)
- **STL Containers**: [std::map](https://en.cppreference.com/w/cpp/container/map), [std::unordered_map](https://en.cppreference.com/w/cpp/container/unordered_map), [std::list](https://en.cppreference.com/w/cpp/container/list)
- **Chrono Library**: [std::chrono](https://en.cppreference.com/w/cpp/chrono)
- **Optional**: [std::optional](https://en.cppreference.com/w/cpp/utility/optional)

### Design Patterns
- **Singleton Pattern**: Ensures single instance
- **RAII**: Resource management through object lifetime
- **Type Aliases**: Improve code readability

---

## 🤝 Contributing

This project demonstrates:
- Clean architecture with separation of concerns
- Extensive use of modern C++ features
- Comprehensive test coverage
- Real-world financial system modeling

---

## 📄 License

MIT

---

## 👤 Author

**Ash4dev** - [GitHub Profile](https://github.com/Ash4dev)

---

## 🙏 Acknowledgments

- [Coding Jesus Orderbook Series](https://www.youtube.com/playlist?list=PLIkrF4j3_p-2C5VuzbBxpBsFzh0qqXtgm) for the idea
- [LearnCPP](https://www.learncpp.com/) for a beginner friendly guide to C++
- [CPPReference](https://cppreference.com/) for crisp and clear reference
- [Mike Shah](https://www.youtube.com/@MikeShah) for the missing parts
- [CppNuts](https://www.youtube.com/@CppNuts) for the missing parts
- [Build systems](https://www.youtube.com/watch?v=2s75npa5IIY) "make"ing it
- [GDB](https://www.youtube.com/watch?v=mfmXcbiRs0E) fixing bugs left and right
- [Google Test](https://github.com/google/googletest) framework for robust testing infrastructure

