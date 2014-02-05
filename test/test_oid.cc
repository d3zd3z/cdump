// Test the OID.

#include <iostream>
#include "gtest/gtest.h"

#include "kind.hh"
#include "oid.hh"
#include "pdump.hh"

namespace {
struct Comp {
  cdump::Kind kind;
  std::string text;
  std::string expect;
};

const std::vector<Comp> cases {
  {"blob", "Simple", "9d91380b823559dd2a4ee5bce3fcc697c56ba3f8"},
  {"zot ", "", "bfc24abdb6cec5eae7d3dd84686117902ad2b562"},
};
}

TEST(OID, Simple) {
  ASSERT_EQ(sizeof(cdump::OID), 20u);

  for (auto& comp : cases) {
    // Test hashing itself.
    cdump::OID oid(comp.kind, comp.text);
    ASSERT_EQ(oid.to_hex(), comp.expect);

    // Test construction from hex.
    cdump::OID oid2(comp.expect);
    ASSERT_EQ(oid2.to_hex(), comp.expect);
  }
}

namespace {
// Test the tweaking case.  If the amount is positive, adjust up,
// otherwise adjust down.
void test_tweaking(const std::string input,
		   const std::string expect,
		   int amount)
{
  cdump::OID oid(input);
  while (amount > 0) {
    ++oid;
    --amount;
  }
  while (amount < 0) {
    --oid;
    ++amount;
  }
  ASSERT_EQ(oid.to_hex(), expect);
}

}

TEST(OID, Tweak) {
  test_tweaking("0000000000000000000000000000000000000000",
		"0000000000000000000000000000000000000001",
		1);
  test_tweaking("0000000000000000000000000000000000000000",
		"0000000000000000000000000000000000000100",
		256);
  test_tweaking("00000000000000000000000000000000ffffffff",
		"0000000000000000000000000000000100000000",
		1);
  test_tweaking("ffffffffffffffffffffffffffffffffffffffff",
		"0000000000000000000000000000000000000000",
		1);

  test_tweaking("ffffffffffffffffffffffffffffffffffffffff",
		"fffffffffffffffffffffffffffffffffffffffe",
		-1);
  test_tweaking("ffffffffffffffffffffffffffffffffffffffff",
		"fffffffffffffffffffffffffffffffffffffeff",
		-256);
  test_tweaking("ffffffffffffffffffffffffffffffff00000000",
		"fffffffffffffffffffffffffffffffeffffffff",
		-1);
  test_tweaking("0000000000000000000000000000000000000000",
		"ffffffffffffffffffffffffffffffffffffffff",
		-1);
}
