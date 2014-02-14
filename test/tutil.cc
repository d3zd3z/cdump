// Testing utilities.

#include <iostream>
#include "gtest/gtest.h"

#include <random>
#include <string>
#include <boost/filesystem.hpp>

#include "tutil.hh"

namespace {
class SimpleRandom {
  uint32_t state;
 public:
  SimpleRandom(unsigned seed) :state(seed) {}
  unsigned next(unsigned limit);
};

unsigned SimpleRandom::next(unsigned limit) {
  state = ((state * 1103515245) + 12345) & 0x7ffffff;
  return state % limit;
}

const std::vector<std::string> word_list {
  "the", "be", "to", "of", "and", "a", "in", "that", "have", "I",
  "it", "for", "not", "on", "with", "he", "as", "you", "do", "at",
  "this", "but", "his", "by", "from", "they", "we", "say", "her",
  "she", "or", "an", "will", "my", "one", "all", "would", "there",
  "their", "what", "so", "up", "out", "if", "about", "who", "get",
  "which", "go", "me", "when", "make", "can", "like", "time", "no",
  "just", "him", "know", "take", "person", "into", "year", "your",
  "good", "some", "could", "them", "see", "other", "than", "then",
  "now", "look", "only", "come", "its", "over", "think", "also" };

}

std::string
make_random_string(unsigned size, unsigned index) {
  std::ostringstream result;
  SimpleRandom gen(index);

  result << index << '-' << size;

  while (result.tellp() < size) {
    result << ' ';
    result << word_list[gen.next(word_list.size())];
  }

  auto buf = result.str();
  buf.resize(size);
  return buf;
}

TEST(TUtil, RandomString) {
  for (unsigned i = 0; i < 256; ++i) {
    const auto line = make_random_string(i, i);
    // std::cout << '|' << line << '|' << std::endl;
    ASSERT_EQ(line.size(), i);
  }
}

cdump::OID int_oid(int index) {
  // Instead of converting the index into ascii, just use the binary
  // representation.  All we really care is that they are different.
  // It seems that the overhead of converting a number to a string is
  // larger than performing the SHA1 hash itself.
  return cdump::OID("blob", &index, sizeof(index));
}

std::vector<unsigned> build_sizes() {
  std::set<unsigned> result;

  for (unsigned i = 0; i < 19; ++i) {
    unsigned bit = 1 << i;
    if (bit > 0)
      result.insert(bit - 1);
    result.insert(bit);
    result.insert(bit + 1);
  }

  std::vector<unsigned> vec;
  std::copy(result.begin(), result.end(), std::back_inserter(vec));
  return vec;
}

cdump::ChunkPtr make_random_chunk(unsigned size, unsigned index) {
  // TODO: How to do this without a copy of the vector.
  auto buf = make_random_string(size, index);
  return cdump::ChunkPtr(new cdump::PlainChunk("blob", buf.data(), buf.size()));
}

//////////////////////////////////////////////////////////////////////
// Tmpdir tests.

void Tmpdir::SetUp() {
  std::random_device rd;  // Maybe global, or better numbers?

  for (int i = 0; i < 5; ++i) {
    std::string work = "/var/tmp/test-";
    for (int j = 0; j < 10; ++j) {
      work += rd() % 26 + 'a';
    }
    auto status = boost::filesystem::create_directory(work);
    if (status) {
      path = work;
      // std::cout << "Tmpdir::SetUp: " << path << std::endl;
      return;
    }
  }
  throw std::runtime_error("Unable to make tmp dir for test");
}

void Tmpdir::TearDown() {
  auto count = boost::filesystem::remove_all(path);
  (void) count;
  // std::cout << "Remove " << count << " entries in " << path << std::endl;
}
