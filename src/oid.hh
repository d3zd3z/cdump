// Object identifiers.

#ifndef __OID_HH__
#define __OID_HH__

#include "kind.hh"

// Nothing yet.
namespace cdump {

struct OID {
 public:
  static const unsigned hash_length = 20;
  static const unsigned textual_length = 2*hash_length;

  // For testing, it is useful to construct from strings.
  OID(Kind kind, std::string data);

  // We can also construct an OID from a hex input string.
  OID(std::string hex);

  // Generate the hex version of the uid.
  std::string to_hex();

 private:
  uint8_t raw[hash_length];
};

}

#endif // __OID_HH__
