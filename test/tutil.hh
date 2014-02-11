// Testing utilities.

#ifndef __TUTIL_HH__
#define __TUTIL_HH__

#include <string>
#include "gtest/gtest.h"
#include "oid.hh"
#include "chunk.hh"

// Make a semi-random string of length 'size', using 'index' as a seed
// for the random generator.
std::string make_random_string(unsigned size, unsigned index);

cdump::ChunkPtr make_random_chunk(unsigned size, unsigned index);

// Generate an OID based on an integer.
cdump::OID int_oid(int index);

// A helper class, with a SetUp and TearDown that make a temporary
// directory for us.
class Tmpdir : public ::testing::Test {
 protected:
  std::string path;
 public:
  virtual void SetUp();
  virtual void TearDown();
};

#endif // __TUTIL_HH__
