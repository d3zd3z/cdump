// Object identifiers.

#ifndef __OID_HH__
#define __OID_HH__

#include <cstring>
#include "kind.hh"

// Nothing yet.
namespace cdump {

struct OID {
 public:
  static const unsigned hash_length = 20;
  static const unsigned textual_length = 2*hash_length;

  // For testing, it is useful to construct from strings.
  OID(Kind kind, std::string data);

  // Often, it will be built of out of a block of data.
  OID(Kind kind, const void* data, size_t size);

  // We can also construct an OID from a hex input string.
  OID(std::string hex);

  // The empty constructor is all zeros.
  OID() {
    memset(raw, 0, hash_length);
  }

  // Copy constructor.
  /*
  OID(const OID& other) {
    memcpy(raw, other.raw, hash_length);
  }
  */

  // Copy assignment.
  OID& operator=(const OID& other) = default;
  /*
  {
    memcpy(raw, other.raw, hash_length);
    return *this;
  }
  */

  // Generate the hex version of the uid.
  std::string to_hex() const;

  // Adjust the current hash by 1, incrementing it (with carry).
  OID& operator--() { tweak(-1, 255); return *this; }
  OID& operator++() { tweak(1, 0); return *this; }

  // Comparison for hashing.
  friend bool operator<(const OID& a, const OID& b) {
    return memcmp(a.raw, b.raw, OID::hash_length) < 0;
  }
  friend bool operator>(const OID& a, const OID& b) {
    return memcmp(a.raw, b.raw, OID::hash_length) > 0;
  }
  friend bool operator==(const OID& a, const OID& b) {
    return memcmp(a.raw, b.raw, OID::hash_length) == 0;
  }

  // Peek at the first byte, needed to build the index.
  unsigned peek_first() const {
    return raw[0];
  }

 private:
  friend struct std::hash<OID>;
  union {
    uint8_t raw[hash_length];

    // This shouldn't be more than a 32-bit value, even though it
    // would seem better to use a larger value for the hash.  But, the
    // hash length is only a multiple of 4, so including an 8-byte
    // possible value here (such as size_t on a 64-bit platform) would
    // result in the whole struct being padded out to a multiple of 8
    // bytes.
    uint32_t user_hash;
  };

  // Adjust the current value by 'adjust', which must be either '1' or
  // '-1'.  The stop value indicates the wraparound.
  void tweak(int adjust, int stop);
};

} // namespace cdump

// Implementation of std::hash for OID.  The OID is already a hash, so
// just use a cast to grab the bytes out of the beginning of it.
namespace std {
template<>
struct hash<cdump::OID> {
  typedef cdump::OID argument_type;
  typedef size_t value_type;

  value_type operator()(argument_type const& oid) const {
    return oid.user_hash;
  }
};
} // namespace std

#endif // __OID_HH__
