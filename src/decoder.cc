// Backup decoder

#include "kind.hh"
#include "decoder.hh"

#include <map>
#include <string>

namespace cdump {

namespace {

// A simplistic decoder.
class Decoder {
  const char* data;
  const char* end;
  unsigned get();
  std::string get_string(unsigned len);
 public:
  Decoder(const char* data, unsigned size)
      : data(data), end(data + size) {}
  bool more() { return data != end; }
  std::string get8();
  std::string get16();
};

unsigned Decoder::get() {
  if (data == end)
    throw std::runtime_error("Invalid encoded block");
  unsigned result = uint8_t(*data);
  ++data;
  return result;
}

std::string Decoder::get_string(unsigned len) {
  std::string result;
  result.reserve(len);
  for (unsigned i = 0; i < len; ++i)
    result += char(get());
  return result;
}

std::string Decoder::get8() {
  unsigned len = get();
  return get_string(len);
}

std::string Decoder::get16() {
  // Big endian value.
  unsigned len = get() << 8;
  len |= get();
  return get_string(len);
}

template<class Func>
void decode_property(const char* data, unsigned size,
		     Func& visitor)
{
  Decoder dec(data, size);
  visitor.set_type(dec.get8());
  while (dec.more()) {
    std::string key = dec.get8();
    std::string value = dec.get16();
    visitor.add_property(key, value);
  }
}

} // namespace

const std::map<Kind, BackupWalk::handler> BackupWalk::handlers {
  { "back", &BackupWalk::walk_back },
};

//////////////////////////////////////////////////////////////////////

// Empty visitors

void BackupVisitor::backup(const OID& root, int64_t date, const property_map& props) {
  (void) root;
  (void) date;
  (void) props;
}

//////////////////////////////////////////////////////////////////////

void BackupWalk::operator()(BackupVisitor& visitor, const OID& root) {
  (void) visitor;

  auto ch = pool.find(root);
  if (!ch)
    throw std::runtime_error("Chunk missing from pool");

  auto pos = handlers.find(ch->kind());
  if (pos == handlers.end()) {
    std::cerr << "Unsupported chunk kind: " << std::string(ch->kind()) << std::endl;
    throw std::runtime_error("Unsupported chunk kind");
  }

  visitor.push_oid(root);
  try {
    (this->*pos->second)(ch, visitor);
  } catch (BackupVisitor::Prune) {
    // Allow.
  }
  visitor.pop_oid();
}

namespace {

class BackNode {
 public:
  std::string type_;
  OID oid;
  int64_t date;
  std::map<std::string, std::string> props;
 public:
  void set_type(std::string type);
  void add_property(std::string key, std::string value);
};

void BackNode::set_type(std::string type) {
  type_ = type;
}

void BackNode::add_property(std::string key, std::string value) {
  if (key == "_date")
    date = stoll(value);
  else if (key == "hash")
    oid = OID(value);
  else
    props[key] = value;
}

} // namespace

void BackupWalk::walk_back(ChunkPtr chunk, BackupVisitor& visitor) {
  BackNode bn;
  decode_property(chunk->data(), chunk->size(), bn);
  visitor.backup(bn.oid, bn.date, bn.props);

  // If Prune was not thrown, walk down to the child.
  operator()(visitor, bn.oid);
}

} // namespace cdump
