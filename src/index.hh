/**
 * File index.
 *
 * This is the file index documentation.
 */

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

// TODO: allow the bool conversion for iterator results.

/**
 * The FileIndex maintains the mapping for a single index file.
 *
 * This class has a RAM index that maintains a temporary mapping.  It
 * is a restricted mapping type, in that most operations aren't
 * supported.
 *
 * The index doesn't itself directly support iteration, but the inner
 * SortedIterator can be used to iterate over the index.
 */
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

  using iterator = value_type*;

 protected:
  // Changes from the disk value are kept here.
  std::unordered_map<OID, Node> ram;

  // The find result is built here.  Note it isn't const, since we
  // need to set it.
  std::pair<key_type, mapped_type> find_result;

  class FileData {
    // The loaded index is stored here.
    std::vector<uint32_t> tops;
    std::vector<OID> hashes;
    std::vector<uint32_t> offsets;
    std::vector<Kind> kind_map;
    std::vector<uint8_t> kinds;

   public:
    void load(const std::string name, uint32_t size);
    bool find(const key_type& key, value_type& result);
    size_t size() const {
      return hashes.size();
    }
    void append_keys(std::vector<OID>& keys) {
      for (const auto& hash : hashes)
	keys.emplace_back(hash);
    }
  };
  FileData fdata;

 public:
  // Differs in that it has no return value (and will raise an
  // exception on error.
  void insert(const value_type& value) {
    (void)ram.insert(value);
  }

  // Looks up a key.  If not found, returns the result of end();
  iterator find(const key_type& key);

  // The iterator telling if the find result is actually found.  There
  // is no begin() because this can't be directly iterated.
  iterator end() {
    return nullptr;
  }

  // Write out this index to the given file.  The 'size' is recorded
  // with the index, and if it doesn't match on 'load', the index will
  // not be used.
  void save(const std::string name, uint32_t size);

  // Load the index.  Obliterates currently loaded data.
  void load(const std::string name, uint32_t size) {
    ram.clear();
    fdata.load(name, size);
  }

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
