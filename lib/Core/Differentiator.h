//===-- Differentiator.h ----------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Class to perform actual execution, hides implementation details from external
// interpreter.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_DIFFERENTIATOR_H
#define KLEE_DIFFERENTIATOR_H

#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <map>
#include <string>

namespace klee {  

/// Return if name matches arg\d\d
bool isSymArg(std::string);

/// Return if name matches out!.*\d
bool isSymOut(std::string);

struct Differentiator {
  std::uint64_t revA, revB;
  // arg_k => v
  std::map<std::uint8_t, std::string> args;
  // k => (out!k!a, out!k!b)
  std::map<std::string, std::pair<std::string, std::string>> outputs;
  // rev => stdout
  std::map<std::uint64_t, std::string> stdouts;
  Differentiator(std::uint64_t a, std::uint64_t b) : revA{a}, revB{b} {}
};

/// Write convenient representation for debugging
llvm::raw_ostream &operator<<(llvm::raw_ostream&, const Differentiator&);
} // End klee namespace

#endif /* KLEE_DIFFERENTIATOR_H */
