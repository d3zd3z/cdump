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
  int count = 0;
  std::cout << "Buckets: " << parent->ram.bucket_count() << "\n";
  for (um::size_type bucket = 0; bucket < parent->ram.bucket_count(); ++bucket) {
    const auto end = parent->ram.end(bucket);
    for (auto it = parent->ram.begin(bucket); it != end; ++it) {
      keys.emplace_back(it->first);
      ++count;
    }
  }

  // Sort all of the keys.
  std::cout << "Gathered " << count << " items\n";
  std::cout << "Sorting\n";
  std::sort(keys.begin(), keys.end());
}

} // namespace cdump
