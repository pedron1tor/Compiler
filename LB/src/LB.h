#pragma once
#include "LA.h"

namespace LB
{
  struct TypeVars
  {
    IR::type var_type;
    std::vector<std::string> vars_declared;
  };
  struct cond
  {
    LA::t left;
    L3::cmp op;
    LA::t right;
  };

  struct ifthen
  {
    cond cnd;
    L3::label then;
    L3::label els;
  };
  struct ReturnTOpt
  {
    std::optional<LA::t> return_thing;
  };
  struct While
  {
    cond cnd;
    L3::label then;
    L3::label els;
  };
  struct Continue
  {
  };
  struct Break
  {
  };
  struct scope;
  using i = std::variant<
      LB::TypeVars, LA::Assign, LA::BinOp, L3::LabelInst, LB::ifthen, L3::Goto, LB::ReturnTOpt, While, Continue, Break, LA::ArrayRead, LA::ArrayWrite,
      LA::Length, LA::CallArbitrary, LA::CallAssignArbitrary, LA::NewArray, LA::NewTuple, std::shared_ptr<LB::scope>>;
  struct scope
  {
    std::vector<LB::i> instructions;
  };
  struct fLB
  {
    IR::type f_type;
    std::string name;
    LA::pars parameters;
    LB::scope lscope;
  };
  struct pLB
  {
    std::vector<LB::fLB> functions;
  };
}