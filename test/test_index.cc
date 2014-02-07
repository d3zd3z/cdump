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
// const int index_count = 10000000;
const int index_count = 1000;

class IndexTracker {
 public:
  std::set<unsigned> inserted;
  cdump::FileIndex index;
  void add(unsigned item) {
    inserted.insert(item);
    // TODO: Test with differing kinds.
    std::pair<cdump::OID, cdump::FileIndex::mapped_type>
	elt(int_oid(item), cdump::FileIndex::mapped_type {item, "blob"});
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
  ASSERT_EQ(res->second.kind, cdump::Kind("blob"));

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
  std::cout << "Making index\n";
  cdump::FileIndex::SortedIterator iter(&index);

  // Test the iterator.
  auto items = inserted;
  cdump::OID last_oid; // Assume we won't ever hash all zeros.
  for (const auto& elt : iter) {
    ASSERT_EQ(elt.second.kind, cdump::Kind("blob"));
    ASSERT_EQ(elt.first, int_oid(elt.second.offset));
    ASSERT_GT(elt.first, last_oid);
    last_oid = elt.first;

    auto count = items.erase(elt.second.offset);
    ASSERT_EQ(count, 1u);
  }
  ASSERT_EQ(items.size(), 0u);
}
} // namespace

TEST(Index, Generate) {
  IndexTracker index;
  std::cout << "Building\n";
  index.add(0, index_count);
  std::cout << "Checking\n";
  index.check_all();

  std::cout << "Testing iter\n";
  index.check_iter();
}

#ifdef OLD_INDEX_TEST
namespace {
// Make this larger to test that the index stuff works with much
// larger values.  This will take longer, though.
// const int index_count = 10000000;
const int index_count = 1000;

// The index has some constraints on the API, specifically, index[key]
// = value isn't supported ([] returns a const ref).

template<typename Indexer>
class IndexTracker {
 public: // Public for testing transparency.
  std::set<unsigned> inserted;
  Indexer index;
 public:
  void add(unsigned item) {
    inserted.insert(item);
    std::pair<cdump::OID, unsigned> elt(int_oid(item), item);
    // cdump::OID oid = int_oid(item);
    // index[oid] = item;
    auto res = index.insert(elt);
    ASSERT_TRUE(res.second);
  }

  // Add [lower, upper) to the structure.
  void add(unsigned lower, unsigned upper) {
    for (unsigned i = lower; i < upper; ++i)
      add(i);
  }

  void check_one(unsigned item);
  void check_all();
};

template<typename Indexer>
void IndexTracker<Indexer>::check_one(unsigned item) {
  cdump::OID good = int_oid(item);
  auto res = index.find(good);
  ASSERT_NE(res, index.end());
  ASSERT_EQ(res->first, good);
  ASSERT_EQ(res->second, item);

  // Verify that tweaked ones aren't found.
  auto alt1 = good;
  ++alt1;
  res = index.find(alt1);
  ASSERT_EQ(res, index.end());

  auto alt2 = good;
  ++alt2;
  res = index.find(alt2);
  ASSERT_EQ(res, index.end());
}

template<typename Indexer>
void IndexTracker<Indexer>::check_all() {
  for (auto it : inserted) {
    check_one(it);
  }
}

}

TEST(Index, RAM) {
  IndexTracker<cdump::FileIndex> indexer;
  indexer.add(0, index_count);
}

#ifdef SHOW_TIMING_TESTS
TEST(Index, Generate) {
  // Just generate a lot of OIDs to see how quickly that is.
  for (int i = 0; i < index_count; ++i) {
    cdump::OID thingy = int_oid(i);
    (void) thingy;
  }
}
#endif // SHOW_TIMING_TESTS

#ifndef SHOW_TIMING_TESTS
TEST(Index, Insert) {
  std::map<cdump::OID, int> index;

  std::cout << "Creating\n";
  for (int i = 0; i < index_count; ++i) {
    cdump::OID thingy = int_oid(i);
    index[thingy] = i;
  }

  // Lets show them.
  /*
  for (auto& it : index) {
    std::cout << it.first.to_hex() << std::endl;
  }
  */
  std::cout << "Freeing\n";
}
#endif // SHOW_TIMING_TESTS

TEST(Index, HashInsert) {
  std::unordered_map<cdump::OID, int> index;

  std::cout << "Creating\n";
  for (int i = 0; i < index_count; ++i) {
    cdump::OID thingy = int_oid(i);
    index[thingy] = i;
  }
  std::cout << "Freeing\n";
}

TEST(Index, Lookup) {
  IndexTracker<std::map<cdump::OID, unsigned>> indexer;

  indexer.add(0, index_count);
  indexer.check_all();
}

// What this hash test shows is that the unordered_map is
// significantly faster on these entries (about 2x as fast), and that
// the overhead to build the array of keys at the end is negligible.
// The memory usage is (surprisingly) similar, although the array of
// keys at the end does take a bit more.
TEST(Index, HashLookup) {
  IndexTracker<std::unordered_map<cdump::OID, unsigned>> indexer;
  indexer.add(0, index_count);
  indexer.check_all();

#if 0
  // To make this "fair", include the time to get all of the keys, and
  // look them up.
  std::vector<cdump::OID> keys;
  for (auto& it : indexer.index) {
    keys.push_back(it.first);
  }
  std::sort(keys.begin(), keys.end());
#endif
}

// Just as a sanity test, try allocating these with references.
TEST(Index, HashPtr) {
  std::unordered_map<std::shared_ptr<cdump::OID>, unsigned> index;

  std::cout << "Creating\n";
  for (unsigned i = 0; i < index_count; ++i) {
    cdump::OID* pitem = new cdump::OID(int_oid(i));
    index[std::shared_ptr<cdump::OID>(pitem)] = i;
  }
  std::cout << "Freeing\n";
}

TEST(Index, TreePtr) {
  std::map<std::shared_ptr<cdump::OID>, unsigned> index;

  std::cout << "Creating\n";
  for (unsigned i = 0; i < index_count; ++i) {
    cdump::OID* pitem = new cdump::OID(int_oid(i));
    index[std::shared_ptr<cdump::OID>(pitem)] = i;
  }
  std::cout << "Freeing\n";
}

#ifdef OLD_TEST
TEST(Index, OldLookup) {
  std::map<cdump::OID, int> index;

  for (int i = 0; i < index_count; ++i) {
    cdump::OID thingy = int_oid(i);
    index[thingy] = i;
  }

  for (int i = 0; i < index_count; ++i) {
    cdump::OID thingy = int_oid(i);
    auto res = index.find(thingy);
    ASSERT_NE(res, index.end());
    ASSERT_EQ(res->first, thingy);
    ASSERT_EQ(res->second, i);

    // Verify that tweaked ones aren't found.
    cdump::OID alt1 = thingy;
    ++alt1;
    res = index.find(alt1);
    ASSERT_EQ(res, index.end());

    cdump::OID alt2 = thingy;
    ++alt2;
    res = index.find(alt1);
    ASSERT_EQ(res, index.end());
  }
}
#endif
#endif // OLD_INDEX_TEST
