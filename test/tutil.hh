// Testing utilities.

#ifndef __TUTIL_HH__
#define __TUTIL_HH__

#include <string>
#include "oid.hh"

// Make a semi-random string of length 'size', using 'index' as a seed
// for the random generator.
std::string make_random_string(unsigned size, unsigned index);

// Generate an OID based on an integer.
cdump::OID int_oid(int index);

#endif // __TUTIL_HH__
