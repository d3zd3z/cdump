// File locking.

#ifndef __LOCKFILE_H__
#define __LOCKFILE_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

namespace cdump {

/**
 * A simple lockfile based on the lockf call.
 */
class LockFile {
  int fd;
  bool locked;
 public:
  LockFile(const char* path) :locked(false) {
    fd = ::open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
      throw std::runtime_error("Unable to open lock file");

    lock();
  }
  ~LockFile() {
    unlock();
    close(fd);
  }

  void lock() {
    if (!locked) {
      auto res = lockf(fd, F_TLOCK, 0);
      if (res != 0)
	throw std::runtime_error("Unable to acquire lock on pool");
      locked = true;
    }
  }

  void unlock() {
    if (locked) {
      auto res = lockf(fd, F_ULOCK, 0);
      if (res != 0)
	std::cerr << "Unable to release lock on pool" << std::endl;
      locked = false;
    }
  }
};

} // namespace cdump

#endif // __LOCKFILE_H__
