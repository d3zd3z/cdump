// Pool testing.

#include "pool.hh"
#include "except.hh"
#include "pdump.hh"
#include "tutil.hh"
#include "gtest/gtest.h"

#include <boost/filesystem.hpp>

#include <cstdlib>

namespace bf = boost::filesystem;

class Pool : public Tmpdir {
  // Use indirection, since this is driven by requests.
  std::unique_ptr<cdump::Pool> pool;
  std::set<unsigned> known;
 public:
  virtual void SetUp();
  virtual void TearDown();

  void create(unsigned limit = cdump::Pool::default_limit,
	      bool newlib = false);
  void open(bool writable = false);
  void close();
  void add(unsigned index);
  void flush();

  void check(unsigned index);

  // Add [low-high) elements.
  void add(unsigned low, unsigned high) {
    for (unsigned i = low; i < high; ++i)
      add(i);
  }

  // Check all of them.
  void check() {
    for (auto elt : known) {
      check(elt);
    }
  }
};

void Pool::SetUp() {
  Tmpdir::SetUp();
  // std::cout << "Pool setup at: " << path << std::endl;
}

void Pool::TearDown() {
  // std::cout << "Pool teardown at: " << path << std::endl;
  Tmpdir::TearDown();
}

void Pool::create(unsigned limit, bool newlib) {
  ASSERT_FALSE(bool(pool));
  cdump::Pool::create_pool(path, limit, newlib);
}

void Pool::open(bool writable) {
  ASSERT_FALSE(bool(pool));
  pool = std::unique_ptr<cdump::Pool>(new cdump::Pool(path, writable));
}

void Pool::close() {
  ASSERT_TRUE(bool(pool));
  delete pool.release();
}

void Pool::flush() {
  ASSERT_TRUE(bool(pool));
  pool->flush();
}

void Pool::add(unsigned index) {
  ASSERT_TRUE(bool(pool));
  ASSERT_EQ(known.count(index), 0u);
  auto ch = make_random_chunk(32, index);
  pool->insert(*ch);
  known.insert(index);
}

void Pool::check(unsigned index) {
  auto ch = make_random_chunk(32, index);
  auto ch2 = pool->find(ch->oid());
  ASSERT_TRUE(bool(ch2));
  ASSERT_EQ(ch->size(), ch2->size());
  ASSERT_EQ(memcmp(ch->data(), ch2->data(), ch->size()), 0);
}

TEST_F(Pool, Creation) {
  create();
  open(true);
  add(1, 2000);
  flush();
  check();

  close();
  open(true);
  check();
  add(2000, 4000);
  check();

  close();
  open(true);
  check();
}

TEST_F(Pool, NewFile) {
  create(cdump::Pool::default_limit, true);
  open(true);
  add(1, 2000);
  close();

  // TODO: Verify that it is present.
}

// TODO: Index recovery.
TEST_F(Pool, IndexRecovery) {
  create();
  open(true);
  add(1, 100);
  close();

  // Stash away the old index.
  bf::path idx = path;
  idx /= "pool-data-0000.idx";
  bf::path idx2 = path;
  idx2 /= "pool-data-0000.idx.orig";

  // This doesn't seem to be implemented.
  // So, do it ourselves.
  // bf::copy(idx, idx2);
  std::string cmd = "cp ";
  cmd += idx.string();
  cmd += ' ';
  cmd += idx2.string();
  // std::cout << cmd << "\n";
  auto res = system(cmd.c_str());
  ASSERT_EQ(res, 0);

  // Add some more nodes.
  open(true);
  add(100, 200);
  close();

  // And put the old index back.
  bf::rename(idx2, idx);

  // And open, which should throw.
  try {
    open();
    FAIL();
  } catch (cdump::index_error) {
    // This is OK.
  }

  // Do index recovery?
  cdump::Pool::recover_index(path);
  open();
  check();
}

#if 0
TEST(Pool, Basic) {
  bool res = boost::filesystem::create_directory("fazzle");
  ASSERT_TRUE(res);
  cdump::Pool::create_pool("fazzle");

  cdump::Pool pool("fazzle", true);
  auto sizes = build_sizes();

  for (auto size : sizes) {
    auto ch = make_random_chunk(size, size);
    pool.insert(ch);
  }
  pool.flush();

  // Verify they could be written.
  for (auto size : sizes) {
    auto ch1 = make_random_chunk(size, size);
    auto ch2 = pool.find(ch1->get_oid());
    ASSERT_TRUE(bool(ch2));
    ASSERT_EQ(ch1->size(), ch2->size());
    ASSERT_EQ(memcmp(ch1->data(), ch2->data(), ch1->size()), 0);
  }
}
#endif

#if 0
TEST(Pool, Creation) {
  // bool res = boost::filesystem::create_directory("test-pool");
  // ASSERT_TRUE(res);
  // (void) res;
  // cdump::Pool::create_pool("test-pool");

  cdump::Pool pool("test-pool");

  auto backs = pool.get_backups();
  /*
  for (const auto& back : backs) {
  */
  const auto& back = backs.back();
    std::cout << back.to_hex() << std::endl;
    auto ch = pool.find(back);
    if (ch) {
      std::cout << "  " << std::string(ch->get_kind())
	  << " " << ch->size()
	  << std::endl;
      pdump::dump(ch->data(), ch->size());
    }
  /*
  }
  */
}
#endif
