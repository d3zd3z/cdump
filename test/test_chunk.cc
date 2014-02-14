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
    cdump::ChunkPtr chunk;
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
  Info& info = stored[ch->get_oid()];
  info.chunk = ch;
  info.offset = buf.tellp();
  ch->write(buf);
}

void ChunkTracker::check_offsets() {
  // Collect all of the chunks, and order them by size.
  // TODO: Figure out how to store references in this vector.
  std::vector<Info> byoffset;

  std::transform(stored.begin(), stored.end(), std::back_inserter(byoffset),
		 [](const std::pair<cdump::OID, Info>&elt) { return elt.second; });

  std::sort(byoffset.begin(), byoffset.end(),
	    [](const Info& a, const Info& b) { return a.offset < b.offset; });

  unsigned pos = 0;
  for (auto& info : byoffset) {
    ASSERT_EQ(pos, info.offset);
    buf.seekg(pos);
    cdump::Chunk::HeaderInfo hinfo;
    auto res = cdump::Chunk::read_header(buf, hinfo);
    ASSERT_TRUE(res);
    ASSERT_EQ(hinfo.kind, info.chunk->get_kind());
    // Compare OID.
    ASSERT_EQ(hinfo.oid, info.chunk->get_oid());
    ASSERT_EQ(hinfo.size, info.chunk->size());

    pos += hinfo.stored_size;
  }
  ASSERT_EQ(pos, buf.tellp());
}

void ChunkTracker::check_read() {
  for (auto& info : stored) {
    buf.seekg(info.second.offset);
    auto ch = cdump::Chunk::read(buf);
    ASSERT_NE(ch, nullptr);
    ASSERT_EQ(ch->get_kind(), info.second.chunk->get_kind());
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
