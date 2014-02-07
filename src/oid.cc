// OID computation.

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include "oid.hh"

#include <openssl/sha.h>

namespace cdump {

OID::OID(Kind kind, std::string data) {
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, kind.textual, 4);
  SHA1_Update(&ctx, data.data(), data.size());
  SHA1_Final(raw, &ctx);
}

OID::OID(Kind kind, const void* data, size_t size) {
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, kind.textual, 4);
  SHA1_Update(&ctx, data, size);
  SHA1_Final(raw, &ctx);
}

OID::OID(std::string hex) {
  if (hex.size() != textual_length) {
    throw std::invalid_argument("OID should be 40-character hex string");
  }

  // buf.exceptions(std::istringstream::failbit);
  for (unsigned i = 0; i < hash_length; ++i) {
    std::istringstream buf(hex.substr(i*2, 2));
    unsigned ch;
    buf >> std::hex >> ch;

    // If we didn't make it to the end, it means a bad character.
    if (!buf.eof())
      throw std::invalid_argument("Invalid hex character in OID hex string");

    raw[i] = ch;
  }
}

std::string OID::to_hex() const {
  std::ostringstream buf;

  for (unsigned i = 0; i < hash_length; ++i) {
    buf << std::hex << std::setfill('0') << std::setw(2) << (unsigned)raw[i];
  }
  return buf.str();
}

void OID::tweak(int adjust, int stop) {
  unsigned pos = hash_length - 1;
  for (;;) {
    uint8_t tmp = raw[pos] + adjust;
    raw[pos] = tmp;
    if (tmp == stop) {
      if (pos == 0)
	// Full wraparound.
	break;
      else
	--pos;
    } else
      break;
  }
}

}
