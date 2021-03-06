// Testing chunks and chunk IO.

#include "chunk.hh"
#include "pdump.hh"
#include "tutil.hh"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>
#include "gtest/gtest.h"

// Verify that compression and decompression directly work.
TEST(Chunk, Compression) {
  using bvec = std::vector<char>;

  // Try something that shouldn't be compressible.
  const std::string in0("Short");
  bvec out0(in0.size(), 0);
  auto len0 = cdump::Chunk::try_compress(in0.data(), in0.size(),
					 out0.data());
  ASSERT_EQ(len0, -1);

  // This string should be compressible.
  const std::string in1("Hello world.  Let's try a much longer string to see if that helps.  Let's try a much longer string to see if that helps.");
  bvec out1(in1.size(), 0);
  auto len1 = cdump::Chunk::try_compress(in1.data(), in1.size(),
					 out1.data());
  ASSERT_NE(len1, -1);
  // pdump::dump(out1.data(), len1);

  // Make sure it can be recompressed.
  bvec full1(in1.size(), 0);
  cdump::Chunk::decompress(out1.data(), len1, full1.data(), full1.size());
}

namespace {
class ChunkTracker {
  struct Info {
    cdump::Chunk::ChunkPtr chunk;
    unsigned offset;
  };

  std::map<cdump::OID, Info> stored;
  std::stringstream buf;
 public:
  void add(unsigned size, unsigned index);
  void check_offsets();
  void check_read();
};

void ChunkTracker::add(unsigned size, unsigned index) {
  auto ch = make_random_chunk(size, index);
  Info& info = stored[ch->oid()];
  info.chunk = std::move(ch);
  info.offset = buf.tellp();
  info.chunk->write(buf);
}

void ChunkTracker::check_offsets() {
  // Collect all of the chunks, and order them by size.
  // TODO: Figure out how to store references in this vector.
  std::vector<Info const*> byoffset;

  std::transform(stored.begin(), stored.end(), std::back_inserter(byoffset),
		 [](std::pair<const cdump::OID, Info>& elt) { return &elt.second; });

  std::sort(byoffset.begin(), byoffset.end(),
	    [](const Info* a, const Info* b) { return a->offset < b->offset; });

  unsigned pos = 0;
  for (auto info : byoffset) {
    ASSERT_EQ(pos, info->offset);
    buf.seekg(pos);
    cdump::Chunk::HeaderInfo hinfo;
    auto res = cdump::Chunk::read_header(buf, hinfo);
    ASSERT_TRUE(res);
    ASSERT_EQ(hinfo.kind, info->chunk->kind());
    // Compare OID.
    ASSERT_EQ(hinfo.oid, info->chunk->oid());
    ASSERT_EQ(hinfo.size, info->chunk->size());

    pos += hinfo.stored_size;
  }
  ASSERT_EQ(pos, buf.tellp());
}

void ChunkTracker::check_read() {
  for (auto& info : stored) {
    buf.seekg(info.second.offset);
    auto ch = cdump::Chunk::read(buf);
    ASSERT_TRUE(bool(ch));
    ASSERT_EQ(ch->kind(), info.second.chunk->kind());
  }
}

} // namespace

TEST(Chunk, IO) {
  ChunkTracker ct;

  for (auto size : build_sizes())
    ct.add(size, size);

  ct.check_offsets();
  ct.check_read();
}

// Verify that the move constructors work.
TEST(Chunk, CopyMove) {
  cdump::PlainChunk ch1("blob", "hello", 5);

  auto ch2 = std::move(ch1);
  ASSERT_EQ(memcmp(ch2.data(), "hello", 4), 0);

  // Not really guaranteed, but conventional.
  ASSERT_EQ(ch1.size(), 0u);

  // This should be a compilation failure.
  // auto ch3 = ch2;
}
