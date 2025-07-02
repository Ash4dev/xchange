#include <gtest/gtest.h>

TEST(LOL, LOL23) { ASSERT_EQ(2, 2); }

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
