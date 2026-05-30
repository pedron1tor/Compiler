#pragma once
#include "IR.h"

namespace LA {
using t = std::variant<L3::N, std::string>;
struct TypeVar {
  IR::type var_type;
  std::string var_declared;
};
struct Assign {
  std::string dest;
  LA::t src;
};
struct BinOp {
  std::string dest;
  LA::t left;
  IR::op operation;
  LA::t right;
};
struct ArrayRead {
  std::string dest;
  std::string src;
  std::vector<LA::t> indices;
  int64_t line;
};
struct ArrayWrite {
  std::string dest;
  std::vector<LA::t> indices;
  LA::t src;
  int64_t line;
};
struct Length {
  std::string dest;
  std::string src;
  std::optional<LA::t> dim;
};
struct NewArray {
  std::string dest;
  std::vector<LA::t> dims;
};
struct NewTuple {
  std::string dest;
  LA::t size;
};
struct CallArbitrary {
  std::string callee;
  std::vector<LA::t> args;
};
struct CallAssignArbitrary {
  std::string dest;
  CallArbitrary callexpr;
};
struct BrCond {
  LA::t cond;
  std::string true_label;
  std::string false_label;
};
struct ReturnT {
  LA::t return_thing;
};
using i = std::variant<
    LA::TypeVar,
    LA::Assign,
    LA::BinOp,
    L3::LabelInst,
    L3::brlabel,
    LA::BrCond,
    L3::Return, LA::ReturnT, LA::ArrayRead, LA::ArrayWrite, LA::Length,
    LA::CallAssignArbitrary,
    LA::CallArbitrary,
    LA::NewArray, LA::NewTuple>;
struct pars {
  std::vector<TypeVar> input_variables;
};
// fLA ::= T name (pars) { i+ }
struct fLA {
  IR::type f_type;
  std::string name;
  LA::pars parameters;
  std::vector<LA::i> instructions;
};

struct pLA {
  std::vector<LA::fLA> functions;
};

} // namespace LA