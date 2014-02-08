// Test driver

#include <iostream>
#include <random>
#include <string>

#include <boost/filesystem.hpp>

#include "gtest/gtest.h"

TEST(PrimitiveTest, WorksAtAll) {
  EXPECT_EQ(1, 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
