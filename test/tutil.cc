// Testing utilities.

#include <iostream>
#include "gtest/gtest.h"

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
