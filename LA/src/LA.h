#pragma once
#include "IR.h"

namespace LA {
struct TypeVar {
  IR::type var_type;
  std::string var_declared;
};
struct Assign {
  std::string dest;
  L3::t src;
};
struct BinOp {
  std::string dest;
  L3::t left;
  IR::op operation;
  L3::t right;
};
struct ArrayRead {
  std::string dest;
  std::string src;
  std::vector<L3::t> indices;
};
struct ArrayWrite {
  std::string dest;
  std::vector<L3::t>
      indices;
  L3::var src;
};
struct Length {
  std::string dest;
  std::string src;
  std::optional<L3::t> dim;
};
struct NewArray {
  std::string dest;
  std::vector<L3::t> dims;
};
struct NewTuple {
  std::string dest;
  L3::t size;
};
struct CallArbitrary {
  std::string callee;
  std::vector<L3::t> args;
};
struct CallAssignArbitrary {
  std::string dest;
  CallArbitrary callexpr;
};
using i = std::variant<
    TypeVar,
    Assign,
    BinOp,
    L3::LabelInst,
    L3::brlabel,
    IR::BrCond,
    L3::Return, L3::ReturnT, ArrayRead, ArrayWrite, Length,
    CallArbitrary, CallAssignArbitrary,
    NewArray, NewTuple,
    >;

// fLA ::= T name (pars) { i+ }
struct fLA {
  IR::type f_type;
  std::string name;
  IR::pars parameters;
  std::vector<i> instructions;
};

struct pLA {
  std::vector<fLA> functions;
};

} // namespace LA