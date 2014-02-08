// Utilities for maintaining the mapping between sha1 hashes and their
// locations in pool files.

#ifndef __INDEX_HH__
#define __INDEX_HH__

#include <cstdint>
#include <unordered_map>
#include <string>
#include <utility>
#include <vector>
#include "kind.hh"
#include "oid.hh"

namespace cdump {

// The FileIndex maintains the mapping for a single index file.  This
// class has a RAM index that maintains a temporary mapping.  It is a
// restricted mapping type, in that most operations aren't supported.
//
// Iteration requires a (relatively) costly build of an intermediate
// data structure.  Insert will erase this, causing the next begin()
// to recreate it.
class FileIndex {
  // Each index entry maps to this node.
 public:
  struct Node {
    uint32_t offset;
    Kind kind;

   public:
    Node() :offset(0), kind("inva") { }
    Node(uint32_t offset, Kind kind) :offset(offset), kind(kind) { }
  };
  using key_type = OID;
  using mapped_type = Node;

  // TODO: This should be a const key_type, but I'm not sure how to
  // build one of these, or perform the cast needed.
  // using value_type = std::pair<const key_type, mapped_type>;
  using value_type = std::pair<key_type, mapped_type>;

  // It is important to realize that 'begin()' and 'find()' invalidate
  // any 'end()' iterator returned.  For normal use, this isn't
  // distinguishable, but you need to be careful.
  using iterator = value_type*;

 protected:
  // Changes from the disk value are kept here.
  std::unordered_map<OID, Node> ram;

  // The find result is built here.  Note it isn't const, since we
  // need to set it.
  std::pair<key_type, mapped_type> find_result;

 public:
  // Differs in that it has no return value (and will raise an
  // exception on error.
  void insert(const value_type& value) {
    (void)ram.insert(value);
  }

  // Looks up a key.  If not found, returns the result of end();
  iterator find(const key_type& key) {
    // Simple ram-only case just looks it up, builds the local result,
    // and returns the pointer.
    const auto fr = ram.find(key);
    if (fr == ram.end())
      return end();
    find_result = *fr;
    return &find_result;
  }

  // The iterator telling if the find result is actually found.  There
  // is no begin() because this can't be directly iterated.
  iterator end() {
    return nullptr;
  }

  // Write out this index to the given file.  The 'size' is recorded
  // with the index, and if it doesn't match on 'load', the index will
  // not be used.
  void save(const std::string name, uint32_t size);

  // The FullIterator iterates the FileIndex in sorted hash order.
  class SortedIterator {
    FileIndex* parent;
   protected:
    std::vector<key_type> keys;
    friend class FileIndex;
   public:
    SortedIterator(FileIndex* parent);
    const std::vector<key_type>& get_keys() { return keys; };

    // The iterator itself returned.
    struct iterator {
      FileIndex* parent;
      decltype(keys)::iterator it;

      friend bool operator==(const iterator& a, const iterator& b) {
	return a.it == b.it;
      }
      friend bool operator!=(const iterator& a, const iterator& b) {
	return a.it != b.it;
      }

      iterator& operator++() {
	++it;
	return *this;
      }

      value_type operator*() {
	return *parent->find(*it);
      }
    };

    const iterator begin() {
      return iterator { parent, keys.begin() };
    }
    const iterator end() {
      return iterator { parent, keys.end() };
    }
  };
};

} // namespace cdump

#endif // __INDEX_HH__
