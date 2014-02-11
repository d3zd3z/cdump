#ifndef __UTILITY_HH__
#define __UTILITY_HH__

#include <vector>
#include <iostream>

namespace cdump {

// Write out the contents of the vector.
template<class E>
void vector_write(std::ostream& out, const std::vector<E>& elts) {
  out.write(reinterpret_cast<const char*>(elts.data()),
	    elts.size() * sizeof(E));
}

// Read a vector.  This is technically not allowed, since the data()
// method returns a const, and explicitly says not to modify the
// underlying array.  But there isn't any other way to do this that
// doesn't involve reading the data in a little at a time.
template<class E>
void vector_read(std::istream& in, std::vector<E>& elts) {
  in.read(reinterpret_cast<char*>(elts.data()),
	  elts.size() * sizeof(E));
}

} // namespace cdump

#endif // __UTILITY_HH__
