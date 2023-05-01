//===-- Differentiator.cpp ------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Differentiator.h"

#include <iomanip>
#include <sstream>

using namespace klee;

namespace klee {

std::string quoted(const std::string& s) {
  std::stringstream ss;
  ss << std::quoted(s);
  return ss.str();
}

inline char hex(unsigned char c) {
    return (c < 10) ? '0' + c : 'a' + c - 10;
}

void writeHex(llvm::raw_ostream& os, std::string s) {
    for (const auto c : s)
        os << "\\x" << hex(c >> 4) << hex(c & 0xf);
}

llvm::raw_ostream &operator<<(llvm::raw_ostream& os, const Differentiator& d) {
  os << "{(";
  uint8_t last = 0;
  for (const auto& s : d.args) {
    if (last++)
      os << " ";
    os << quoted(s);
  }
  os << ") {";
  last = 0;
  for (const auto& p : d.outputs) {
    os << (last++ ? " :" : ":") << p.first << " {"; os << d.revA << " ";
    writeHex(os, p.second.first);
    os << " " << d.revB << " ";
    writeHex(os, p.second.second);
    os << "}";
  }
  os << "}}";

  return os;
}
}
