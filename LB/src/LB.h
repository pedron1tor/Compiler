#pragma once
#include "LA.h"
#include <functional>

namespace LB {
struct TypeVars {
  IR::type var_type;
  std::vector<std::string> vars_declared;
};
struct cond {
  LA::t left;
  IR::op op;
  LA::t right;
};

struct ifthen {
  cond cnd;
  L3::label then;
  L3::label els;
};
struct ReturnTOpt {
  std::optional<LA::t> return_thing;
};
struct While {
  cond cnd;
  L3::label then;
  L3::label els;
  bool operator==(const While &o) const {
    return then.name == o.then.name && els.name == o.els.name;
  }
};
struct WhileHasher {
  size_t operator()(const While &w) const {
    size_t h1 = std::hash<std::string>{}(w.then.name);
    size_t h2 = std::hash<std::string>{}(w.els.name);
    return h1 ^ (h2 << 1);
  }
};
struct WhileInfo {
  std::unordered_map<While, L3::label, WhileHasher> begin_while;
  std::unordered_map<While, L3::label, WhileHasher> end_while;
  std::unordered_map<While, L3::label, WhileHasher> cond_labels;
  std::unordered_map<int, const While *> loop;
};
struct Continue {
};
struct Break {
};
struct scope;
using i = std::variant<
    LB::TypeVars, LA::Assign, LA::BinOp, L3::LabelInst, LB::ifthen, L3::Goto,
    LB::ReturnTOpt, LB::While, LB::Continue, LB::Break, LA::ArrayRead, LA::ArrayWrite,
    LA::Length, LA::CallArbitrary, LA::CallAssignArbitrary, LA::NewArray,
    LA::NewTuple, std::shared_ptr<LB::scope>>;
struct scope {
  std::vector<LB::i> instructions;
};
struct fLB {
  IR::type f_type;
  std::string name;
  LA::pars parameters;
  std::shared_ptr<LB::scope> lscope;
};
struct pLB {
  std::vector<LB::fLB> functions;
};
} // namespace LB