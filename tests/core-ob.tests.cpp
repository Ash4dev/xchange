#include "include/Xchange.hpp"
#include <gtest/gtest.h>

TEST(LOL, LOL44) { ASSERT_EQ(3, 3); }

TEST(LOL2, LOL23) { ASSERT_EQ(2, 2); }
TEST(LOL2, LOL233) { ASSERT_EQ(2, 2); }

TEST(Xchage, SetUpCheck) {
  Xchange &xchange = Xchange::getInstance(3, 1000);
  Xchange::destroyInstance();
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
