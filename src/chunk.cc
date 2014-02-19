// Chunks.

#include "chunk.hh"
#include "utility.hh"

#include <stdexcept>
#include <zlib.h>

namespace cdump {

/**
 * Attempt to compress a block of data.
 *
 * Attempts to compress the buffer pointed to at `src`, containing
 * `src_len` bytes of data, into the buffer at `dest`.  `dest` is
 * assumed to contains at least as many bytes of space as `src_len`.
 *
 * @param src the source buffer
 * @param src_len the number of bytes of the source data
 * @param dest the destination buffer
 * @return the count of the number of bytes written to dest, or -1 if
 * the compressed data would be larger than the source.
 */
int Chunk::try_compress(const char* src, unsigned src_len,
			 char* dest)
{
  // Don't bother trying if less than 16 bytes.  zlib fails with weird
  // errors, and it wouldn't help to compress it, anyway, since the
  // blocks are padded to 16 bytes.
  if (src_len < 16)
    return -1;

  uLongf dest_len = src_len;
  int res = ::compress2(reinterpret_cast<Bytef*>(dest), &dest_len,
			reinterpret_cast<const Bytef*>(src), src_len,
			3);
  if (res == Z_OK)
    return dest_len;
  else if (res == Z_BUF_ERROR)
    return -1;
  else
    throw std::runtime_error("Error compressing with zlib");
}

/**
 * Decompress a block of data.
 *
 * Reverses the Chunk::try_compress above.  The dest_len must exactly
 * match the expected size of the decompression, or this will result
 * in an error.
 *
 * @param src the source buffer (compressed data)
 * @param src_len the source data length
 * @param dest the destination buffer
 * @param dest_len the size available for decompression
 */
void Chunk::decompress(const char* src, unsigned src_len,
		       char* dest, unsigned dest_len)
{
  uLongf pdest_len = dest_len;
  int res = ::uncompress(reinterpret_cast<Bytef*>(dest), &pdest_len,
			 reinterpret_cast<const Bytef*>(src), src_len);
  if (res != Z_OK || pdest_len != dest_len)
    throw std::runtime_error("Error decompressing with zlib");
}

namespace {
const int magic_size = 16;
const char* magic = "adump-pool-v1.1\n";

struct Header {
  char magic[magic_size];
  int32_t clen; //< Length of stored data in file.
  int32_t uclen; //< Length of uncompressed data, or -1 if not compressed.
  Kind kind;
  OID oid;
};

const char padding[] = {0};

// Round a size up to the next size increment.
unsigned padded(unsigned size) {
  return (size + 15) & ~15;
}
} // namespace

void Chunk::write(std::ostream& out) const {
  Header head;
  memcpy(head.magic, magic, magic_size);
  head.uclen = htole32(size());
  head.kind = kind_;
  head.oid = oid_;
  const char* payload;
  uint32_t payload_len;
  if (has_zdata()) {
    payload = zdata();
    payload_len = zsize();
    head.uclen = htole32(size());
  } else {
    payload = data();
    payload_len = size();
    head.uclen = htole32(-1);
  }
  head.clen = htole32(payload_len);
  out.write(reinterpret_cast<const char*>(&head), sizeof(head));
  out.write(payload, payload_len);

  const unsigned pad_len = 15 & -payload_len;
  if (pad_len > 0)
    out.write(padding, pad_len);
}

// TODO: Test this function.
unsigned Chunk::write_size() const {
  if (has_zdata()) {
    return padded(sizeof(Header) + zsize());
  } else {
    return padded(sizeof(Header) + size());
  }
}

bool Chunk::read_header(std::istream& in, HeaderInfo& info) {
  Header head;
  in.read(reinterpret_cast<char*>(&head), sizeof(head));
  if (memcmp(head.magic, magic, magic_size) != 0)
    return false;
  info.kind = head.kind;
  info.oid = head.oid;
  unsigned payload_len = le32toh(head.clen);
  int32_t uclen = le32toh(head.uclen);
  if (uclen == -1) {
    info.size = payload_len;
  } else {
    info.size = uclen;
  }
  info.stored_size = padded(sizeof(head) + payload_len);
  return true;
}

Chunk::ChunkPtr Chunk::read(std::istream& in) {
  Header head;
  in.read(reinterpret_cast<char*>(&head), sizeof(head));
  if (memcmp(head.magic, magic, magic_size) != 0)
    throw new std::runtime_error("Incorrect chunk header");
  int clen = le32toh(head.clen);
  int uclen = le32toh(head.uclen);

  if (uclen == -1) {
    return ChunkPtr(new PlainChunk(head.kind, head.oid, in, clen));
  } else
    return ChunkPtr(new CompressedChunk(head.kind, head.oid, in,
					uclen, clen));
}

// Construct from given data.
PlainChunk::PlainChunk(const Kind kind, const char* data, unsigned data_len)
  :Chunk(kind, data, data_len),
    zdata_info(Untried)
{
  plain_data.resize(data_len);
  memcpy(plain_data.data(), data, data_len);
}

// Construct by reading data from a file.
PlainChunk::PlainChunk(const Kind kind, const OID& oid, std::istream& in, unsigned data_len)
  :Chunk(kind, oid)
{
  plain_data.resize(data_len);
  vector_read(in, plain_data);
}

const char*
PlainChunk::data() const {
  return plain_data.data();
}

unsigned
PlainChunk::size() const {
  return plain_data.size();
}

bool
PlainChunk::has_zdata() const {
  switch (zdata_info) {
    case None:
      return false;
    case Some:
      return true;
    case Untried:
      // Although we are declared as const, modified the cached
      // compressed value to avoid additional computation.
      PlainChunk* wthis = const_cast<PlainChunk*>(this);

      wthis->compressed_data.resize(plain_data.size());
      int res = try_compress(plain_data.data(), plain_data.size(),
			     wthis->compressed_data.data());
      if (res < 0) {
	wthis->zdata_info = None;
	wthis->compressed_data.clear();
	wthis->compressed_data.shrink_to_fit();
	return false;
      } else {
	wthis->zdata_info = Some;
	wthis->compressed_data.resize(res);
	// Not sure if the shrink_to_fit is worth it.  It may copy the
	// data again.
	// compressed_data.shrink_to_fit();
	return true;
      }
  }
  abort();
}

const char*
PlainChunk::zdata() const {
  if (!has_zdata())
    throw std::runtime_error("Attempt to get zdata from incompressible chunk");
  return compressed_data.data();
}

unsigned
PlainChunk::zsize() const {
  if (!has_zdata())
    throw std::runtime_error("Attempt to get zsize from incompressible chunk");
  return compressed_data.size();
}

// Compressed chunks.
CompressedChunk::CompressedChunk(const Kind kind, const OID& oid, std::istream& in,
				 unsigned data_len, unsigned zdata_len)
  :Chunk(kind, oid),
    data_len(data_len),
    is_decompressed(false)
{
  compressed_data.resize(zdata_len);
  vector_read(in, compressed_data);
}

const char*
CompressedChunk::data() const {
  if (!is_decompressed) {
    // Although we are declared as const, modified the cached
    // uncompressed value to avoid additional computation.
    CompressedChunk* wthis = const_cast<CompressedChunk*>(this);
    wthis->plain_data.resize(data_len);
    decompress(compressed_data.data(), compressed_data.size(),
	       wthis->plain_data.data(), wthis->plain_data.size());
    wthis->is_decompressed = true;
  }
  return plain_data.data();
}

unsigned
CompressedChunk::size() const {
  return data_len;
}

bool
CompressedChunk::has_zdata() const {
  return true;
}

const char*
CompressedChunk::zdata() const {
  return compressed_data.data();
}

unsigned
CompressedChunk::zsize() const {
  return compressed_data.size();
}

} // namespace cdump
