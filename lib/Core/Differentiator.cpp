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
bool isSymArg(std::string name) {
  return (name.size() == 5 // string::starts_with requires C++20
          && name[0] == 'a' && name[1] == 'r' && name[2] == 'g'
          && '0' <= name[3] && name[3] <= '9'
          && '0' <= name[4] && name[4] <= '9');
}

bool isSymOut(std::string name) {
  // string::starts_with requires C++20
  return (name[0] == 'o' && name[1] == 'u' && name[2] == 't' && name[3] == '!'
          && '0' <= name[name.size() - 1] && name[name.size() - 1] <= '9');
}

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
  for (const auto& p : d.args) {
    assert(p.first == last);
    if (last)
      os << " ";
    os << quoted(p.second);
    last++;
  }
  os << ") {";
  last = 0;
  for (const auto& p : d.outputs) {
    os << (last ? " :" : ":") << p.first << " {"; os << d.revA << " ";
    writeHex(os, p.second.first);
    os << " " << d.revB << " ";
    writeHex(os, p.second.second);
    os << "}";
    last++;
  }
  os << "}}";

  return os;
}
}
