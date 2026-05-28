#pragma once
#include "L3.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace IR {
// type ::= int64([])* | tuple | code
struct t_int64 {
  int dim;
};
struct t_tuple {
};
struct t_code {
};
struct t_void {
};

// T ::= type | void
using type = std::variant<t_void, t_tuple, t_code, t_int64>;

// op ::= + | - | * | & | << | >> | < | <= | = | >= | >
enum class op {
  add,
  sub,
  mul,
  and_,
  lshift,
  rshift,
  lt,
  gt,
  le,
  gte,
  eq
};

// pars ::= type var | type var (, type var)* |
struct TypeVar {
  type var_type;
  L3::var var_declared;
};

// var <- t op t
struct BinOp {
  L3::var dest;
  L3::t left;
  op operation;
  L3::t right;
};

// var <- var([t])+
struct ArrayRead {
  L3::var dest;
  L3::var src;
  std::vector<L3::t> indices;
};

// var([t])+ <- s
struct ArrayWrite {
  L3::var dest;
  std::vector<L3::t> indices;
  L3::s src;
};

// var <- length var t | var <- length var
struct Length {
  L3::var dest;
  L3::var src;
  std::optional<L3::t> dim;
};

// var <- new Array(args)
struct NewArray {
  L3::var dest;
  std::vector<L3::t> dims;
};

// var <- new Tuple(t)
struct NewTuple {
  L3::var dest;
  L3::t size;
};

// te ::= br t label label
struct BrCond {
  L3::t cond;
  L3::label true_label;
  L3::label false_label;
};

using i = std::variant<
    L3::Assign,
    BinOp,
    L3::CallL3, L3::CallAssignL3,
    L3::CallPrint, L3::CallInput,
    L3::CallTupleError, L3::CallTensorError,
    ArrayRead, ArrayWrite, Length, NewArray, NewTuple,
    TypeVar>;

// te ::= br label | br t label label | return | return t
using te = std::variant<L3::Goto, BrCond, L3::Return, L3::ReturnT>;

// bb ::= label i* te
struct bb {
  L3::label lbl;
  std::vector<i> instructions;
  te terminal;
  bool operator<(const bb &other) const { return lbl.name < other.lbl.name; }
};

// pars ::= type var (, type var)* | ε
struct pars {
  std::vector<TypeVar> input_variables;
};

// fIR ::= define T l (pars) { bb+ }
struct fIR {
  type f_type;
  L3::l name;
  pars parameters;
  std::vector<bb> blocks;
};

struct pIR {
  std::vector<fIR> functions;
};

struct trace_bb {
  L3::label lbl;
  std::vector<i> instructions;
  te terminal;
  bool mark;
};
struct trace {
  std::vector<trace_bb> blocks;
};
} // namespace IR
