//===-- FunctionStateInfo.cpp ---------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "FunctionStateInfo.h"

namespace klee {
FunctionStateInfo::~FunctionStateInfo() {}

void FunctionStateInfo::print(llvm::raw_ostream &out) const {
  for (std::map<llvm::Function *, std::string>::const_iterator
         it = stateInfoMap.begin(), ie = stateInfoMap.end();
       it != ie; ++it)
    out << it->second << "\n";
}

void FunctionStateInfo::addStateInfo(llvm::Function *callee, std::string info) {
    stateInfoMap.erase(callee);
    stateInfoMap[callee] = info;
}
} /* namespace klee */
