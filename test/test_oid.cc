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
  ASSERT_EQ(sizeof(cdump::OID), 20);

  for (auto& comp : cases) {
    // Test hashing itself.
    cdump::OID oid(comp.kind, comp.text);
    ASSERT_EQ(oid.to_hex(), comp.expect);

    // Test construction from hex.
    cdump::OID oid2(comp.expect);
    ASSERT_EQ(oid2.to_hex(), comp.expect);
  }
}
