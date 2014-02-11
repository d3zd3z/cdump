// Index operations.

#include "index.hh"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "kind.hh"
#include "utility.hh"

namespace cdump {

FileIndex::SortedIterator::SortedIterator(FileIndex* parent) :parent(parent) {
  keys.reserve(parent->ram.size() + parent->fdata.size());

  // Build the keys out of the ram index.
  for (auto& it : parent->ram) {
    keys.emplace_back(it.first);
  }

  // Add add the keys from the non-ram index.
  parent->fdata.append_keys(keys);

  // Sort all of the keys.
  std::sort(keys.begin(), keys.end());
}

namespace {

const int magic_size = 8;
const char magic[] = "ldumpidx";
const int magic_version = 4;

struct Header {
  char magic[magic_size];
  uint32_t version;
  uint32_t file_size;
};

// After loading a vector, fix the endianness.
void fix_vector_endian(std::vector<uint32_t> elts) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  (void) elts;  // Nothing
#else
  for (auto& elt : elts) {
    elt = le32toh(elt);
  }
#endif
}

class Saver {
  // FileIndex *index;
  FileIndex::SortedIterator iter;
  std::vector<uint32_t> tops;
  std::vector<uint32_t> offsets;
  std::set<Kind> kinds;
  std::map<Kind, uint32_t> kind_map;

  void compute_tops();
  void first_pass();
  void write_kinds(std::ostream& out);
 public:
  Saver(FileIndex *index) :/*index(index),*/ iter(index) {
    compute_tops();
  }

  void save(const std::string name, uint32_t size);
};

void Saver::save(const std::string name, uint32_t size) {
  std::ofstream file(name, std::ios::binary|std::ios::out);
  file.exceptions(file.badbit|file.failbit);

  Header head;
  memcpy(head.magic, magic, magic_size);
  head.version = htole32(magic_version);
  head.file_size = htole32(size);
  file.write(reinterpret_cast<char*>(&head), sizeof(head));
  vector_write(file, tops);
  vector_write(file, iter.get_keys());

  first_pass();
  vector_write(file, offsets);
  write_kinds(file);
}

void Saver::compute_tops() {
  tops.resize(256, 0);

  auto it = iter.begin();
  uint32_t num = 0;

  for (unsigned top = 0; top < 256; ++top) {
    // Find the first hash with a first byte greater than 'top'.
    // TODO: Fix iterator so that it->first will work.
    while (it != iter.end() && (*it).first.peek_first() <= top) {
      ++it;
      ++num;
    }
    tops[top] = num;
  }
}

// Pass one through the hashes computes the offsets, and fills in the
// available kinds.
void Saver::first_pass() {
  // TODO: Make iter have a size() method.
  offsets.clear();
  offsets.reserve(iter.get_keys().size());
  kinds.clear();
  for (const auto& elt : iter) {
    offsets.push_back(htole32(elt.second.offset));
    kinds.insert(elt.second.kind);
  }

  // From this, construct the kind map.
  unsigned k = 0;
  for (auto kind = kinds.begin();
       kind != kinds.end();
       ++kind, ++k)
  {
    kind_map[*kind] = k;
  }
}

// Writing kinds, writes out the map, and then the items themselves.
void Saver::write_kinds(std::ostream& out) {
  const uint32_t num_kinds = kinds.size();
  out.write(reinterpret_cast<const char*>(&num_kinds), sizeof(num_kinds));

  std::vector<Kind> knames;
  knames.reserve(num_kinds);
  for (const auto k : kinds) {
    knames.push_back(k);
  }
  vector_write(out, knames);

  // All of the kinds are stored as single bytes.
  std::vector<uint8_t> kbytes;
  kbytes.reserve(iter.get_keys().size());
  for (const auto& elt : iter) {
    kbytes.push_back(kind_map[elt.second.kind]);
  }
  vector_write(out, kbytes);
}

} // namespace

void FileIndex::save(const std::string name, uint32_t size) {
  const auto tmp = name + ".tmp";
  Saver saver(this);
  saver.save(tmp, size);
  const int result = std::rename(tmp.c_str(), name.c_str());
  if (result != 0) {
    throw std::ios::failure("Unable to rename tmp file");
  }
}

FileIndex::iterator FileIndex::find(const FileIndex::key_type& key) {
  // Simple ram-only case just looks it up, builds the local result,
  // and returns the pointer.
  const auto fr = ram.find(key);
  if (fr == ram.end()) {
    if (fdata.find(key, find_result))
      return &find_result;
    else
      return end();
  }
  find_result = *fr;
  return &find_result;
}

// Searching of loaded data.
bool FileIndex::FileData::find(const FileIndex::key_type& key, FileIndex::value_type& result) {
  if (tops.size() != 256)
    return false;

  const auto first = key.peek_first();

  // It is important that these are both signed, since high can go
  // negative.
  int low = (first > 0) ? tops[first-1] : 0;
  int high = tops[first] - 1;

  while (high >= low) {
    unsigned mid = low + ((high - low) / 2);
    auto cmp = key.cmp(hashes[mid]);
    if (cmp < 0)
      high = mid - 1;
    else if (cmp > 0)
      low = mid + 1;
    else {
      // It matches, return the result.
      result.first = key;
      result.second.offset = offsets[mid];
      result.second.kind = kind_map[kinds[mid]];
      return true;
    }
  }

  (void) result;
  return false;
}

//////////////////////////////////////////////////////////////////////
// Loading

void FileIndex::FileData::load(const std::string name, uint32_t size) {
  std::ifstream file(name, std::ios::binary|std::ios::in);
  if (!file.good()) {
    throw std::ios::failure("Unable to read index file");
  }
  file.exceptions(file.badbit|file.failbit|file.eofbit);

  Header head;
  file.read(reinterpret_cast<char*>(&head), sizeof(head));
  if (memcmp(head.magic, magic, magic_size) != 0)
    throw std::ios::failure("Index header has invalid magic");
  if (le32toh(head.version) != magic_version)
    throw std::ios::failure("Index file incorrect version");
  if (le32toh(head.file_size) != size)
    throw std::ios::failure("Index file incorrect size");

  // Reading the data in.  Vector doesn't give us write access to the
  // underlying data...  But, we can actually write to it anyway.
  tops.resize(256);
  vector_read(file, tops);
  fix_vector_endian(tops);

  const unsigned count = tops[255];
  hashes.resize(count);
  vector_read(file, hashes);

  offsets.resize(count);
  vector_read(file, offsets);
  fix_vector_endian(offsets);

  uint32_t kind_count;
  file.read(reinterpret_cast<char*>(&kind_count), sizeof(kind_count));
  kind_count = le32toh(kind_count);

  kind_map.resize(kind_count);
  vector_read(file, kind_map);

  kinds.resize(count);
  vector_read(file, kinds);
}

} // namespace cdump
