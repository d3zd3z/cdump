// Testing the index code.

#include "index.hh"

#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <memory>
#include <unordered_map>
#include "gtest/gtest.h"

#include "tutil.hh"
#include "oid.hh"

namespace {
// Make this larger to test that the index stuff works with much
// larger values.  This will take longer, though.
// const int index_count = 1000000;
const int index_count = 1000;

// For testing, we need to map the integer index to various kinds.  To
// keep the distribution interesting, use this very simple hash to
// scramble the input sequence number.
unsigned scramble(unsigned x) {
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x) * 0x45d9f3b;
  x = ((x >> 16) ^ x);
  return x;
}

const std::vector<cdump::Kind> kind_table {
  "blob", "dir ", "dir1", "dir2", "dir3", "ind0", "ind1", "ind2", "back"
};

cdump::Kind kind_of(unsigned sequence) {
  return kind_table[scramble(sequence) % kind_table.size()];
}

class IndexTracker {
 public:
  std::set<unsigned> inserted;
  cdump::FileIndex index;
  void add(unsigned item) {
    inserted.insert(item);
    // TODO: Test with differing kinds.
    std::pair<cdump::OID, cdump::FileIndex::mapped_type>
	elt(int_oid(item), cdump::FileIndex::mapped_type {item, kind_of(item)});
    index.insert(elt);
  }

  // Add [lower, upper) to the structure.
  void add(unsigned lower, unsigned upper) {
    for (unsigned i = lower; i < upper; ++i)
      add(i);
  }

  void check_one(unsigned item);
  void check_all();

  // Verify that the iterator iterators everything properly.
  void check_iter();
};

void IndexTracker::check_one(unsigned item) {
  cdump::OID good = int_oid(item);
  auto res = index.find(good);
  ASSERT_NE(res, index.end());
  ASSERT_EQ(res->first, good);
  ASSERT_EQ(res->second.offset, item);
  ASSERT_EQ(res->second.kind, kind_of(item));

  // Verify that tweaked hashes aren't findable.
  cdump::OID alt1 = good;
  ++alt1;
  res = index.find(alt1);
  ASSERT_EQ(res, index.end());

  cdump::OID alt2 = good;
  --alt2;
  res = index.find(alt2);
  ASSERT_EQ(res, index.end());
}

void IndexTracker::check_all() {
  for (auto it : inserted)
    check_one(it);
}
void IndexTracker::check_iter() {
  // Build the iterator out of it.
  // std::cout << "Making index\n";
  cdump::FileIndex::SortedIterator iter(&index);

  // Test the iterator.
  auto items = inserted;
  cdump::OID last_oid; // Assume we won't ever hash all zeros.
  for (const auto& elt : iter) {
    ASSERT_EQ(elt.second.kind, kind_of(elt.second.offset));
    ASSERT_EQ(elt.first, int_oid(elt.second.offset));
    ASSERT_GT(elt.first, last_oid);
    last_oid = elt.first;

    auto count = items.erase(elt.second.offset);
    ASSERT_EQ(count, 1u);
  }
  ASSERT_EQ(items.size(), 0u);
}
} // namespace

class IndexTest : public Tmpdir {
};

TEST_F(IndexTest, Generate) {
  const std::string name1 = path + "/sample.idx";
  const std::string name2 = path + "/sample2.idx";

  IndexTracker index;
  // std::cout << "Building\n";
  index.add(0, index_count);
  // std::cout << "Checking\n";
  index.check_all();

  // std::cout << "Testing iter\n";
  index.check_iter();

  // Write it out.
  index.index.save(name1, index.inserted.size());

  // And load it back in.
  index.index.load(name1, index.inserted.size());

  // Testing that.
  index.check_all();
  index.check_iter();

  // Add some more.
  index.add(index_count, index_count*2);
  index.check_all();
  index.check_iter();

  // Save and restore.
  index.index.save(name2, index.inserted.size());
  index.index.load(name2, index.inserted.size());
  index.check_all();
  index.check_iter();
}
