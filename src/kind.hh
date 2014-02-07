// Chunk kinds.
//
// Every chunk has a kind value, which is a 32-bit value represented
// as a 4-character string.

#ifndef __SRC_KIND_HH__
#define __SRC_KIND_HH__

#include <cstdint>
#include <cstring>
#include <string>

namespace cdump {

// Check that this is a valid kind.
void kind_check(const char* cstr);
void kind_check(const std::string& cstr);

struct Kind {
  friend struct OID;
 protected:
  // The kind is really just a 32-bit unsigned integer, and the
  // compiler should be smart enough to treat it as such.  But, we
  // want to be able to treat it as a string.  It is represented with
  // native endianness, so that it will output properly.
  union {
    uint32_t raw;
    char     textual[4];
  };

 public:
  Kind(const char* cstr) {
    kind_check(cstr);
    std::memcpy(&raw, cstr, 4);
  }

  Kind(const std::string& str) {
    kind_check(str);
    std::memcpy(&raw, str.data(), 4);
  }

  // Implicit conversion to string.
  operator std::string() {
    std::string result(textual, 4);
    return result;
  }

  // Comparisons.
  friend bool operator==(const Kind& a, const Kind& b) {
    return a.raw == b.raw;
  }
};

}

#endif // __SRC_KIND_HH__
