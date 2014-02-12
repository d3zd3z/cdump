// Pool testing.

#include "pool.hh"
#include "pdump.hh"
#include "gtest/gtest.h"

#include <boost/filesystem.hpp>

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
