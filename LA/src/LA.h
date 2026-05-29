#pragma once
#include "IR.h"

namespace LA
{
  struct TypeVar
  {
    IR::type var_type;
    std::string var_declared;
  };
  struct Assign
  {
    std::string dest;
    L3::t src;
  };
  struct BinOp
  {
    std::string dest;
    L3::t left;
    IR::op operation;
    L3::t right;
  };
  struct ArrayRead
  {
    std::string dest;
    std::string src;
    std::vector<L3::t> indices;
  };
  struct ArrayWrite
  {
    std::string dest;
    std::vector<L3::t>
        indices;
    L3::var src;
  };
  struct Length
  {
    std::string dest;
    std::string src;
    std::optional<L3::t> dim;
  };
  struct NewArray
  {
    std::string dest;
    std::vector<L3::t> dims;
  };
  struct NewTuple
  {
    std::string dest;
    L3::t size;
  };
  using i = std::variant<
      Assign,
      BinOp,
      L3::CallL3, L3::CallAssignL3,
      L3::CallPrint, L3::CallInput,
      L3::CallTupleError, L3::CallTensorError,
      ArrayRead, ArrayWrite, Length, NewArray, NewTuple,
      TypeVar, L3::Return, L3::ReturnT>;

  // fLA ::= T name (pars) { i+ }
  struct fLA
  {
    IR::type f_type;
    std::string name;
    IR::pars parameters;
    std::vector<i> instructions;
  };

  struct pLA
  {
    std::vector<pLA> functions;
  };

} // namespace LA