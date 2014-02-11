// Chunk kinds.

#include <stdexcept>
#include "kind.hh"

namespace cdump {

/**
 * Validate the passed kind.
 *
 * Ensure that the kind passed in is a valid string, and is exactly 4
 * characters long.
 *
 * @param cstr the kind name.
 * @throws std::invalid_argument when the name is invalid.
 */
void kind_check(const char* cstr) {
  if (cstr == nullptr || strlen(cstr) != 4)
    throw std::invalid_argument("Invalid kind");
}

/**
 * Validate the passed kind.
 *
 * @param str a std::string to check.
 * @see kind_check above.
 */
void kind_check(const std::string& str) {
  if (str.size() != 4)
    throw std::invalid_argument("Invalid kind length");
}

}
