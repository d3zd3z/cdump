// Exceptions used inside the pool code.

#ifndef __EXCEPT_HH__
#define __EXCEPT_HH__

#include <stdexcept>
#include <string>

namespace cdump {

class pool_open_error : public std::runtime_error {
 public:
  explicit pool_open_error(const std::string& arg)
      : std::runtime_error(arg) {}
  virtual ~pool_open_error() {}
};

class index_error : public std::runtime_error {
 public:
  explicit index_error(const std::string& arg)
      : std::runtime_error(arg) {}
  virtual ~index_error() {}
};

}

#endif // __EXCEPT_HH__
