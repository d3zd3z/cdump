// Main driver for cdump.

#include "decoder.hh"
#include "pool.hh"
#include "pdump.hh"

#include <iostream>
#include <iomanip>
#include <map>
#include <string>

class Lister : public cdump::BackupVisitor {
  struct Node {
    cdump::OID self_oid;
    int64_t date;
    property_map props;

    Node(const cdump::OID& self_oid, int64_t date, const property_map& props)
	:self_oid(self_oid),
	date(date),
	props(props) {}
  };

  std::vector<Node> backups;

 public:
  void sort();
  void show();

  virtual void backup(const cdump::OID& root, int64_t date, const property_map& props);
};

void Lister::backup(const cdump::OID& root, int64_t date, const property_map& props) {
  (void) root;
  backups.emplace_back(peek_oid(), date, props);
  throw prune;
}

typedef std::pair<std::string, std::string> entry;

void Lister::show() {
  sort();

  for (const auto& ent : backups) {
    std::cout << ent.self_oid.to_hex()
	<< ' '
	<< ent.date;
    for (auto& kv : ent.props) {
      std::cout << ' ' << kv.first << '=' << kv.second;
    }
    std::cout << std::endl;
  }
}

void Lister::sort() {
  std::sort(backups.begin(), backups.end(),
	    [](const Node& a, const Node& b) { return a.date < b.date; });
}

int main() {
  cdump::Pool pool("test-pool");
  std::cout << "Hello world\n";

  auto backups = pool.get_backups();
  std::cout << "There are " << backups.size() << " backups\n";

  Lister lister;
  cdump::BackupWalk walk(pool);
  for (auto& oid : backups) {
    walk(lister, oid);
    // lister.add(oid);
  }
  lister.show();
}
