//===-- ExprUtil.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "klee/Expr/ExprUtil.h"
#include "klee/Expr/Expr.h"
#include "klee/Expr/ExprHashMap.h"
#include "klee/Expr/ExprVisitor.h"

#include <algorithm>
#include <set>

using namespace klee;

void klee::findReads(ref<Expr> e, 
                     bool visitUpdates,
                     std::vector< ref<ReadExpr> > &results) {
  // Invariant: \forall_{i \in stack} !i.isConstant() && i \in visited 
  std::vector< ref<Expr> > stack;
  ExprHashSet visited;
  std::set<const UpdateNode *> updates;
  
  if (!isa<ConstantExpr>(e)) {
    visited.insert(e);
    stack.push_back(e);
  }

  while (!stack.empty()) {
    ref<Expr> top = stack.back();
    stack.pop_back();

    if (ReadExpr *re = dyn_cast<ReadExpr>(top)) {
      // We memoized so can just add to list without worrying about
      // repeats.
      results.push_back(re);

      if (!isa<ConstantExpr>(re->index) &&
          visited.insert(re->index).second)
        stack.push_back(re->index);
      
      if (visitUpdates) {
        // XXX this is probably suboptimal. We want to avoid a potential
        // explosion traversing update lists which can be quite
        // long. However, it seems silly to hash all of the update nodes
        // especially since we memoize all the expr results anyway. So
        // we take a simple approach of memoizing the results for the
        // head, which often will be shared among multiple nodes.
        if (updates.insert(re->updates.head.get()).second) {
          for (const auto *un = re->updates.head.get(); un;
               un = un->next.get()) {
            if (!isa<ConstantExpr>(un->index) &&
                visited.insert(un->index).second)
              stack.push_back(un->index);
            if (!isa<ConstantExpr>(un->value) &&
                visited.insert(un->value).second)
              stack.push_back(un->value);
          }
        }
      }
    } else if (!isa<ConstantExpr>(top)) {
      Expr *e = top.get();
      for (unsigned i=0; i<e->getNumKids(); i++) {
        ref<Expr> k = e->getKid(i);
        if (!isa<ConstantExpr>(k) &&
            visited.insert(k).second)
          stack.push_back(k);
      }
    }
  }
}

///

namespace klee {

class SymbolicObjectFinder : public ExprVisitor {
protected:
  Action visitRead(const ReadExpr &re) {
    const UpdateList &ul = re.updates;

    // XXX should we memo better than what ExprVisitor is doing for us?
    for (const auto *un = ul.head.get(); un; un = un->next.get()) {
      visit(un->index);
      visit(un->value);
    }

    if (ul.root->isSymbolicArray())
      if (results.insert(ul.root).second)
        objects.push_back(ul.root);

    return Action::doChildren();
  }

public:
  std::set<const Array*> results;
  std::vector<const Array*> &objects;
  
  SymbolicObjectFinder(std::vector<const Array*> &_objects)
    : objects(_objects) {}
};

ExprVisitor::Action ConstantArrayFinder::visitRead(const ReadExpr &re) {
  const UpdateList &ul = re.updates;

  // FIXME should we memo better than what ExprVisitor is doing for us?
  for (const auto *un = ul.head.get(); un; un = un->next.get()) {
    visit(un->index);
    visit(un->value);
  }

  if (ul.root->isConstantArray()) {
    results.insert(ul.root);
  }

  return Action::doChildren();
}
}

template<typename InputIterator>
void klee::findSymbolicObjects(InputIterator begin, 
                               InputIterator end,
                               std::vector<const Array*> &results) {
  SymbolicObjectFinder of(results);
  for (; begin!=end; ++begin)
    of.visit(*begin);
}

void klee::findSymbolicObjects(ref<Expr> e,
                               std::vector<const Array*> &results) {
  findSymbolicObjects(&e, &e+1, results);
}

std::uint64_t pickPatchNo(std::uint64_t m, std::uint64_t n) {
  // 0 means original, uint64_t max means merged.
  return (0ull < n && n < 0xffffffffffffffffull) ? n : m;
}

std::vector<std::pair<std::uint64_t, ref<Expr>>>
klee::splitExpr(const ref<Expr>& value) {
  const auto& expr = value.get();
  std::vector<std::pair<std::uint64_t, ref<Expr>>> res {};
  if (!expr)
    return res;
  if (!expr->meta) {
    res.push_back(std::make_pair(0, value));
    return res;
  }

  switch (expr->getKind()) {
  case Expr::NotOptimized:
    for (const auto& src : splitExpr(static_cast<NotOptimizedExpr&>(*expr).src))
      res.push_back(std::make_pair(src.first,
                                   NotOptimizedExpr::create(src.second)));
    break;
  case Expr::Read: {
    const auto& read = static_cast<ReadExpr&>(*expr);
    for (const auto& index : splitExpr(read.index))
      res.push_back(std::make_pair(index.first,
                                   ReadExpr::create(read.updates,
                                                    index.second)));
  } break;
  case Expr::Select: {
    const auto& select = static_cast<SelectExpr&>(*expr);
    if (select.merge) {
      for (const auto& truePair : splitExpr(select.trueExpr))
        res.push_back(std::make_pair(pickPatchNo(select.truePatch,
                                                 truePair.first),
                                     truePair.second));
      for (const auto& falsePair : splitExpr(select.falseExpr))
        res.push_back(std::make_pair(pickPatchNo(select.falsePatch,
                                                 falsePair.first),
                                     falsePair.second));
    } else for (const auto& cond : splitExpr(select.cond))
      for (const auto& trueExpr : splitExpr(select.trueExpr))
        for (const auto& falseExpr : splitExpr(select.falseExpr)) {
          auto patchNo = pickPatchNo(cond.first, pickPatchNo(trueExpr.first,
                                                             falseExpr.first));
          res.push_back(std::make_pair(patchNo,
                                       SelectExpr::create(cond.second,
                                                          trueExpr.second,
                                                          falseExpr.second)));
        }
  } break;
  case Expr::Concat: {
    const auto& concat = static_cast<ConcatExpr&>(*expr);
    for (const auto& left : splitExpr(concat.getLeft()))
      for (const auto& right : splitExpr(concat.getRight()))
        res.push_back(std::make_pair(pickPatchNo(left.first, right.first),
                                     ConcatExpr::create(left.second,
                                                        right.second)));
  } break;
  case Expr::Extract: {
    const auto& extract = static_cast<ExtractExpr&>(*expr);
    for (const auto& e : splitExpr(extract.expr))
      res.push_back(std::make_pair(e.first,
                                   ExtractExpr::create(e.second, extract.offset,
                                                       extract.width)));
  } break;
  case Expr::ZExt: {
    const auto& zext = static_cast<ZExtExpr&>(*expr);
    for (const auto& src : splitExpr(zext.src))
      res.push_back(std::make_pair(src.first,
                                   ZExtExpr::create(src.second, zext.width)));
  } break;
  case Expr::SExt: {
    const auto& sext = static_cast<SExtExpr&>(*expr);
    for (const auto& src : splitExpr(sext.src))
      res.push_back(std::make_pair(src.first,
                                   SExtExpr::create(src.second, sext.width)));
  } break;
#define SPLIT_AL_EXPR(_class_kind) case Expr::_class_kind: {                   \
    const auto& op = static_cast<_class_kind##Expr&>(*expr);                   \
    for (const auto& left : splitExpr(op.left))                                \
      for (const auto& right : splitExpr(op.right))                            \
        res.push_back(std::make_pair(pickPatchNo(left.first, right.first),     \
                                     _class_kind##Expr::create(left.second,    \
                                                               right.second)));\
  } break;
  SPLIT_AL_EXPR(Add)
  SPLIT_AL_EXPR(Sub)
  SPLIT_AL_EXPR(Mul)
  SPLIT_AL_EXPR(UDiv)
  SPLIT_AL_EXPR(SDiv)
  SPLIT_AL_EXPR(URem)
  SPLIT_AL_EXPR(SRem)
  SPLIT_AL_EXPR(And)
  SPLIT_AL_EXPR(Or)
  SPLIT_AL_EXPR(Xor)
  SPLIT_AL_EXPR(Shl)
  SPLIT_AL_EXPR(LShr)
  SPLIT_AL_EXPR(AShr)
  SPLIT_AL_EXPR(Eq)
  SPLIT_AL_EXPR(Ne)
  SPLIT_AL_EXPR(Ult)
  SPLIT_AL_EXPR(Ule)
  SPLIT_AL_EXPR(Ugt)
  SPLIT_AL_EXPR(Uge)
  SPLIT_AL_EXPR(Slt)
  SPLIT_AL_EXPR(Sle)
  SPLIT_AL_EXPR(Sgt)
  SPLIT_AL_EXPR(Sge)
#undef SPLIT_AL_EXPR
  case Expr::Not:
    for (const auto& e : splitExpr(static_cast<ExtractExpr&>(*expr).expr))
      res.push_back(std::make_pair(e.first, NotExpr::create(e.second)));
    break;
  case Expr::Constant:
    res.push_back(std::make_pair(0, value));
    break;
  default:
    assert(0 && "invalid expression kind");
  }
  return res;
}

typedef std::vector< ref<Expr> >::iterator A;
template void klee::findSymbolicObjects<A>(A, A, std::vector<const Array*> &);

typedef std::set< ref<Expr> >::iterator B;
template void klee::findSymbolicObjects<B>(B, B, std::vector<const Array*> &);
