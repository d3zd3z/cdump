// Test driver

#include <iostream>
#include <random>
#include <string>

#include <boost/filesystem.hpp>

#include "gtest/gtest.h"

TEST(PrimitiveTest, WorksAtAll) {
  EXPECT_EQ(1, 1);
}

class TmpdirTest : public ::testing::Test {
  std::string path;
 public:
  virtual void SetUp();
  virtual void TearDown();
};

void TmpdirTest::SetUp() {
  std::random_device rd; // Maybe use global, or better numbers?

  for (int i = 0; i < 5; ++i) {
    std::string work = "/var/tmp/test-";
    for (int j = 0; j < 10; ++j) {
      work += rd() % 26 + 'a';
    }
    auto status = boost::filesystem::create_directory(work);
    if (status) {
      path = work;
      return;
    }
  }
  throw std::runtime_error("Unable to make tmp dir for test");
}

void TmpdirTest::TearDown() {
  auto count = boost::filesystem::remove_all(path);
  (void) count;
  // std::cout << "Remove " << count << " entries in " << path << std::endl;
}

TEST_F(TmpdirTest, UsableDir) {
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
