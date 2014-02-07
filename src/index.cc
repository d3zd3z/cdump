// Index operations.

#include "index.hh"
#include <algorithm>
#include <iostream>

namespace cdump {

FileIndex::SortedIterator::SortedIterator(FileIndex* parent) :parent(parent) {
  // using um = std::unordered_map<OID, Node>;
  using um = decltype(ram);

  keys.reserve(parent->ram.size());

  // Build the keys out of the ram index.
  for (auto& it : parent->ram) {
    keys.emplace_back(it.first);
  }

  // Sort all of the keys.
  std::sort(keys.begin(), keys.end());
}

} // namespace cdump
