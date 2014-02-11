// Hexdumping utility.

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "pdump.hh"

namespace pdump {

// It's "easier" to encode the hex ourselves.
namespace {
const char hex[] = "0123456789abcdef";
}

class Dumper {
  std::ostringstream hex;
  std::ostringstream ascii;

  const uint8_t* data;
  const size_t size;

  void reset();
  void add(unsigned pos);
  void ship(unsigned pos);
 public:
  Dumper(const void* data, size_t size)
      :data((const uint8_t*) data),
      size(size) { }
  void dump();
};

void Dumper::reset() {
  hex.str(std::string());
  hex.clear();
  ascii.str(std::string());
  ascii.clear();
}

void Dumper::dump() {
  const auto ending = (size + 15) & -15;
  for (unsigned pos = 0; pos < ending; ++pos) {
    add(pos);
    if ((pos & 15) == 7)
      hex << ' ';
    if ((pos & 15) == 15)
      ship(pos & ~15);
  }
}

void Dumper::add(unsigned pos) {
  if (pos < size) {
    const auto ch = data[pos];
    hex << ' ' << std::hex << std::setfill('0') << std::setw(2) << (unsigned)ch;
    if (std::isprint(ch))
      ascii << (char)ch;
    else
      ascii << '.';
  } else {
    hex << "   ";
    ascii << ' ';
  }
}

void Dumper::ship(unsigned pos) {
  std::cout << std::hex << std::setfill('0') << std::setw(6) << pos
      << ' ' << hex.str() << " |" << ascii.str() << '|' << std::endl << std::dec;
  reset();
}

void dump(const void* data, size_t size) {
  Dumper d(data, size);
  d.dump();
}

}
