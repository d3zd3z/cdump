// Backup chunks.

#ifndef __CHUNK_H__
#define __CHUNK_H__

#include "oid.hh"
#include <cstddef>
#include <memory>
#include <vector>
#include <iostream>

namespace cdump {

class Chunk;

/**
 * Backup chunk
 *
 * Chunks are the fundamental unit of backup.  Everything is made out
 * of, or broken up into chunks.
 *
 * Each chunk consists of a 4-character kind field (4 8-bit
 * characters, meaning they should really only be 7-bit characters to
 * avoid encoding problems), and zero or more bytes of data.
 *
 * Chunks inherently support compression of their payload, and are
 * handled in both compressed and uncompressed form.  Generally, the
 * uncompressed format will represent the real backup data, and the
 * compressed version will be used for network transfer or the storage
 * pool.
 */
class Chunk {
 protected:
  // Simple fields.
  Kind kind_;
  OID oid_;

 protected:
  // Construct based on some data.
  Chunk(const Kind kind, const char* data, unsigned data_len)
      :kind_(kind)
  {
    oid_ = OID(kind, data, data_len);
  }

  // Construct with already known OID.
  Chunk(const Kind kind, const OID oid)
      :kind_(kind), oid_(oid) {}

  // No copying allowed.  It isn't particularly hard, but we want to
  // make sure copies are avoided.
  Chunk(const Chunk& other) = delete;
  Chunk& operator=(const Chunk& other) = delete;

 public:
  virtual ~Chunk() {}

  // Chunks can be moved, not copied.
  Chunk(Chunk&& other) = default;
  Chunk& operator=(Chunk&& other) = default;

  // Used for polymorphic return.
  typedef std::unique_ptr<Chunk> ChunkPtr;

  using data_type = std::vector<char>;

  // TODO: don't use get_xxx() names for getters, just use the name.

  /// Get the object ID for this chunk.
  OID const& oid() const { return oid_; }

  /// Get the `Kind` for this chunk.
  Kind kind() const { return kind_; }

  /// Get the uncompressed data of this chunk.
  virtual const char* data() const = 0;

  /**
   * Get the length of the uncompressed data.
   *
   * This is separate because a Chunk may know the length of the
   * uncompressed data without having to decompress it.
   */
  virtual unsigned size() const = 0;

  /**
   * Determine if this chunk is compressible.
   */
  virtual bool has_zdata() const = 0;

  /**
   * Attempt to get the compressed payload.
   *
   * If the data is compressible, return a pointer to the compressed
   * data.  Otherwise, return a nullptr to indicate that the data is
   * not compressible.
   */
  virtual const char* zdata() const = 0;

  /// Get the size of the compressed data.
  virtual unsigned zsize() const = 0;

  /**
   * Write this chunk out to the given ostream.
   *
   * The stream should be opened in binary mode.
   */
  void write(std::ostream& out) const;

  /**
   * Determine how many bytes it will take to write this chunk out.
   */
  unsigned write_size() const;

  struct HeaderInfo {
    Kind kind;
    OID  oid;
    unsigned size; // Data size of chunk.
    unsigned stored_size; // Offset to next chunk in the file.
  };
  /**
   * Try to read the header of the chunk from the istream.
   *
   * Returns true if the chunk could be read, and fills in the info
   * with the information about the chunk.
   */
  static bool read_header(std::istream& in, HeaderInfo& info);

  /**
   * Attempt to read a chunk from the stream.
   *
   * Reads the chunk
   */
  static ChunkPtr read(std::istream& in);

  // static ChunkPtr read(std::istream& in, 

  // These are not intended to be used externally, but are exported for
  // testing.
  static int try_compress(const char* src, unsigned src_len,
			  char* dest);
  static void decompress(const char* src, unsigned src_len,
			 char* dest, unsigned dest_len);
};

/**
 * Chunk taken from uncompressed data.
 */
class PlainChunk : public Chunk {
  std::vector<char> plain_data;
  std::vector<char> compressed_data;

  enum ZDataInfo {
    Untried,
    None,
    Some
  };
  ZDataInfo zdata_info;
 public:
  PlainChunk(const Kind kind, const char* data, unsigned data_len);
  PlainChunk(const Kind kind, const OID& oid, std::istream& in, unsigned data_len);

  virtual const char* data() const override;
  virtual unsigned size() const override;
  virtual bool has_zdata() const override;
  virtual const char* zdata() const override;
  virtual unsigned zsize() const override;
};

/**
 * Chunk taken from compressed data.
 */
class CompressedChunk : public Chunk {
  unsigned data_len;
  std::vector<char> plain_data;
  std::vector<char> compressed_data;
  bool is_decompressed;

 public:
  CompressedChunk(const Kind kind, const OID& oid, std::istream& in, unsigned data_len, unsigned zdata_len);

  virtual const char* data() const override;
  virtual unsigned size() const override;
  virtual bool has_zdata() const override;
  virtual const char* zdata() const override;
  virtual unsigned zsize() const override;
};

} // namespace cdump

#endif // __CHUNK_H__
