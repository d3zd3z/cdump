// Testing utilities.

#ifndef __TUTIL_HH__
#define __TUTIL_HH__

#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "oid.hh"
#include "chunk.hh"

// Make a semi-random string of length 'size', using 'index' as a seed
// for the random generator.
std::string make_random_string(unsigned size, unsigned index);

cdump::ChunkPtr make_random_chunk(unsigned size, unsigned index);

// Generate an OID based on an integer.
cdump::OID int_oid(int index);

/**
 * Build a vector of useful sizes to test.  The result is a sorted
 * vector containing all of the powers of two, up until 256K, and one
 * less, and one greater than each of those values.
 */
std::vector<unsigned> build_sizes();

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
