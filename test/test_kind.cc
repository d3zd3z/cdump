// Test kind transforms.

#include <cstring>
#include <string>

#include "gtest/gtest.h"

#include "kind.hh"

using namespace cdump;

TEST(Kind, Simple) {
  Kind k1 { "blob" };
  ASSERT_EQ(std::string(k1), "blob");

  const std::string blob { "blob" };
  Kind k2 { blob };
  ASSERT_EQ(std::memcmp(std::string(k2).data(), "blob", 4), 0);
}
