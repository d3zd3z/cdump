// Chunk kinds.

#include <stdexcept>
#include "kind.hh"

namespace cdump {

// Validate a cstring passed as a kind.
void kind_check(const char* cstr) {
  if (cstr == nullptr || strlen(cstr) != 4)
    throw std::invalid_argument("Invalid kind");
}

// Validate a string passed as a kind.
void kind_check(const std::string& str) {
  if (str.size() != 4)
    throw std::invalid_argument("Invalid kind length");
}

}
