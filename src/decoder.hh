// Backup decoder.

#ifndef __DECODER_HH__
#define __DECODER_HH__

#include "kind.hh"
#include "pool.hh"

#include <forward_list>
#include <map>
#include <string>

namespace cdump {

/**
 * The visitor is called for each node of the backup as the backup is
 * traversed.
 */
class BackupVisitor {
 public:
  typedef std::map<std::string, std::string> property_map;

 public:
  /**
   * Any of the visitor methods can throw Prune to prune the tree at a
   * given level, and avoid descending this node's children.
   */
  class Prune {};

  // A prune singleton for ease of use.
  static const Prune prune;

 protected:
  // The visitor tracks its location through this stack of oids.  The
  // visiting pushes the current oid before calling methods, and pops
  // them upon return.
  std::forward_list<OID> oids;

  void push_oid(const OID& oid) {
    oids.push_front(oid);
  }

  void pop_oid() {
    oids.pop_front();
  }

  friend class BackupWalk;

 public:
  const OID& peek_oid() {
    return oids.front();
  }

 public:
  // Each of these functions will be called as the tree is traversed.

  virtual void backup(const OID& root, int64_t date, const property_map& props);
};

/**
 * A class for traversing a backup.
 */
class BackupWalk {
  Pool& pool;
 public:
  BackupWalk(Pool& pool) : pool(pool) {}

  /**
   * Walk the backup starting with the given OID of the root, invoking
   * the visitor for each visited node.
   */
  void operator()(BackupVisitor& visitor, const OID& root);

 private:
  typedef void (BackupWalk::* handler)(ChunkPtr chunk, BackupVisitor& visitor);
  static const std::map<Kind, handler> handlers;

  // Handlers for the various types.
  void walk_back(ChunkPtr chunk, BackupVisitor& visitor);
};

} // namespace cdump

#endif // __DECODER_HH__
