// Storage pools.

#ifndef __POOL_HH__
#define __POOL_HH__

#include <string>
#include <fstream>
#include <vector>
#include <forward_list>

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>

#include "lockfile.hh"
#include "index.hh"
#include "chunk.hh"
#include "oid.hh"

namespace cdump {

/**
 * A Pool stores backup `Chunk`s in a series of files contained in a
 * single directory.
 *
 * The pool class virtually wraps alternate implementations of the
 * pool (such as files, or remotely accessed).
 */
class Pool {
  const boost::filesystem::path base;
  const bool writable;

  // The file descriptor of the lock file.
  std::string lock_path();
  LockFile lock;

  // The properties from the backup.
  struct Props {
    boost::uuids::uuid uuid;
    bool newfile;
    unsigned limit;
  };
  Props props;
  void read_props(const std::string path);

  // `props.newfile` gets copied here, and this is cleared, since
  // newfile only applies the first time a write happens.
  bool first_newfile;

  // Within each pool, we have zero or more files.  At most, the last
  // file can be open for writing.  Generally, the intermediate ones
  // will be opened only for reading.
  struct File {
    unsigned     pos;
    std::fstream file;
    FileIndex    index;
    unsigned     size;

    File(const Pool& parent, unsigned pos, bool create = false);
    void make_writable(const Pool& parent);
    void unmake_writable(const Pool& parent);
  };

  // All of the pool files, in reverse order (pushed at the front).
  // Using a forward_list allows unmovable items to be built directly
  // inside.
  std::forward_list<File> files;

  void scan_files();
  void recover_files();

  // Indicates we've started writing.  When true, files.front().file
  // will be opened for writing, and write_pos will be set to the
  // position to write into that file.
  bool dirty = false;
  unsigned write_pos = 0;

  // Prepare to write `needed` bytes of data.  When finished, 'dirty'
  // will be tru, and files.front().file will be opened for write, and
  // write_pos set to the position in that file to write at.
  void prepare_write(unsigned needed);

  std::string construct_name(unsigned pos, const std::string extension) const;

  // Private constructor.
  Pool(const std::string path, bool writable, bool recover);

 public:
  /**
   * Attempt to open a pool with the given path.
   *
   * The pool must already exist.
   * @param path the pathname to the directory containing the pool.
   * @param writable indicates if this pool should be writable.
   */
  Pool(const std::string path, bool writable = false)
    : Pool(path, writable, false) {}

  ~Pool() {
    flush();
  }

  /// The default limit on the size of a file for the pool.  640 MB
  /// fits on a CD, with 7 fitting on a DVD.
  static const unsigned default_limit = 640 * 1024 * 1024;
  static const unsigned limit_lower_bound = 1 << 20;
  static const unsigned limit_upper_bound = (1 << 30) - 1;

  /**
   * Create a new file pool in an empty directory.
   *
   * @param path the pathname to the directory to create the pool in.
   * @param limit the largest size in bytes a single file can grow to.
   * @param newfile if true, indicates that each time the pool is
   * opened, data should be written to a new file.
   * @throws std::runtime_error if the pool cannot be created.
   */
  static void create_pool(const std::string path,
			  unsigned limit = default_limit,
			  bool newlib = false);

  /**
   * Attempt to recover the index files for a given pool.  Must be
   * able to write to the pool (the lock is held).
   */
  static void recover_index(const std::string path);

  bool is_writable() { return writable; }

  /**
   * Each backup pool records the OID's of top-level backups.
   * Retrieve a list of these.
   */
  std::vector<OID> get_backups() const;

  /**
   * Attempt to read a chunk from the pool.  Throws a ___ exception if
   * the chunk couldn't be found.
   */
  Chunk::ChunkPtr find(const OID& key);

  /**
   * Insert the given chunk into the storage pool.
   */
  void insert(Chunk const& chunk);
  void flush();
};

} // namespace cdump

#endif // __POOL_HH__
