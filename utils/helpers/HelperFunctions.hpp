#include "utils/alias/Fundamental.hpp"
#include "utils/enums/Actions.hpp"
#include "utils/enums/OrderTypes.hpp"
#include "utils/enums/Side.hpp"

#include <optional>
#include <string>

// Template functions need to be defined in header files, not in .cpp files,
// because templates need to be instantiated at compile time.

// convert str types to T optional through long long
template <typename T>
std::optional<T> parseOptionalNumeric(const std::string &str) {
  if (str == "-" || str.empty())
    return std::nullopt;
  else
    return static_cast<T>(std::stoll(str)); // str -> ll -> T
}

// convert str to str optional
std::optional<std::string> parseOptionalString(const std::string &str);

// parses string in given datetime fmt (opt to std::tm obj)
TimeStamp parseTimeStamp(const std::string &dateTime);

Actions::Actions parseAction(const std::string &str);

Side::Side parseSide(const std::string &str);

OrderType::OrderType parseOrderType(const std::string &typeString);
