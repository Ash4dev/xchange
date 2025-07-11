
#include <gtest/gtest.h>

TEST(LOL, LOL1) { ASSERT_EQ(1, 1); }

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}
