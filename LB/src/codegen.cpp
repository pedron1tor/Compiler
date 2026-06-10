#include "LA.h"
#include "parser.h"
#include "parsing/primitives.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_map>
#include <utils.h>

namespace LB {

bool f_type_void(LA::fLA &f) {
  return std::visit([&](auto &func) {
    using T = std::decay_t<decltype(func)>;
    if constexpr (std::is_same_v<T, IR::t_void>)
      return true;
    else
      return false;
  },
                    f.f_type);
};
bool inst_is_labelinst(LA::i &instr) {
  return std::visit([&](auto &ins) {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, L3::LabelInst>)
      return true;
    else
      return false;
  },
                    instr);
};
bool is_declaration(LA::i &inst, std::vector<IR::TypeVar> &variable_accumulator) {
  return std::visit([&](auto &ins) -> bool {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, LA::TypeVar>) {
      IR::TypeVar type_var = IR::TypeVar{ins.var_type, L3::var{ins.var_declared}};
      variable_accumulator.push_back(type_var);
      return true;
    } else {
      return false;
    }
  },
                    inst);
};

static IR::i LA_to_IRinst(LA::i &inst, const std::set<std::string> &code_vars, const std::unordered_map<std::string, IR::type> &type_map) {
  return std::visit([&](auto &ins) -> IR::i {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, LA::Assign>) {
      L3::s src = la_t_to_l3_s(ins.src);
      if (code_vars.count(ins.dest)) {
        if (auto *v = std::get_if<L3::var>(&src); v && !code_vars.count(v->name))
          src = L3::s{L3::l{"@" + v->name}};
      }
      return L3::Assign{L3::w{L3::var{ins.dest}}, src};
    } else if constexpr (std::is_same_v<T, LA::BinOp>)
      return IR::BinOp{L3::var{ins.dest}, la_t_to_l3_t(ins.left), ins.operation, la_t_to_l3_t(ins.right)};
    else if constexpr (std::is_same_v<T, LA::ArrayRead>)
      return IR::ArrayRead{L3::var{ins.dest}, L3::var{ins.src}, la_t_vec_to_l3_t_vec(ins.indices)};
    else if constexpr (std::is_same_v<T, LA::ArrayWrite>) {
      L3::s src = la_t_to_l3_s(ins.src);
      if (auto *v = std::get_if<L3::var>(&src); v && !type_map.count(v->name))
        src = L3::s{L3::l{"@" + v->name}}; // add @ annotation if the variable is a function
      return IR::ArrayWrite{L3::var{ins.dest}, la_t_vec_to_l3_t_vec(ins.indices), src};
    } else if constexpr (std::is_same_v<T, LA::Length>)
      return IR::Length{L3::var{ins.dest}, L3::var{ins.src},
                        ins.dim ? std::optional<L3::t>{la_t_to_l3_t(*ins.dim)} : std::nullopt};
    else if constexpr (std::is_same_v<T, LA::NewArray>)
      return IR::NewArray{L3::var{ins.dest}, la_t_vec_to_l3_t_vec(ins.dims)};
    else if constexpr (std::is_same_v<T, LA::NewTuple>)
      return IR::NewTuple{L3::var{ins.dest}, la_t_to_l3_t(ins.size)};
    else if constexpr (std::is_same_v<T, LA::CallAssignArbitrary>)
      return L3::CallAssignL3{L3::var{ins.dest},
                              L3::CallL3{str_to_callee(ins.callexpr.callee, code_vars),
                                         la_t_vec_to_l3_t_vec(ins.callexpr.args)}};
    else if constexpr (std::is_same_v<T, LA::CallArbitrary>)
      return L3::CallL3{str_to_callee(ins.callee, code_vars), la_t_vec_to_l3_t_vec(ins.args)};
    else
      return IR::TypeVar{IR::type{IR::t_void{}}, L3::var{""}};
  },
                    inst);
}
void variable_declaration(std::vector<IR::TypeVar> &all_var_accumulator, std::vector<IR::bb> &blocks) {
  if (blocks.size() > 0) {
    std::vector<IR::i> prefix;
    for (auto &typed_var : all_var_accumulator)
      prefix.push_back(IR::i{typed_var});
    for (auto &typed_var : all_var_accumulator) {
      prefix.push_back(IR::i{L3::Assign{typed_var.var_declared, L3::N{0}}});
    }
    blocks[0].instructions.insert(blocks[0].instructions.begin(), prefix.begin(), prefix.end());
  }
};

IR::fIR fLA_to_fIR(LA::fLA &f) {

  for (auto &instr : f.instructions) {
  }
  IR::fIR result;
  result.f_type = f.f_type;
  result.name = L3::l{"@" + f.name};
  // result.parameters = ir_pars;
  // result.blocks = new_insts;
  return result;
}

template <typename T, typename F> // why is there no map in cpp???? .......
auto map(const std::vector<T> &vec, F f) {
  std::vector<decltype(f(vec[0]))> result;
  result.reserve(vec.size());
  for (const auto &x : vec) {
    result.push_back(f(x));
  }
  return result;
}

LA::pLA pLB_to_pLA(LB::pLB &p) {
  std::vector<LA::fLA> function_list;

  for (auto &fn : p.functions) {
    // function_list.push_back(fLA_to_fIR(fn));
  }
  return LA::pLA{
      function_list};
};

void generate_code(pLB &p) {
  LA::pLA transformed_p;
  transformed_p = pLB_to_pLA(p);
  std::ofstream outputFile;
  outputFile.open("prog.IR");

  // // build the IR code for each of the funtions inside of the ast/program
  for (auto &fn : transformed_p.functions) {
    print_functionLA(fn, true, &outputFile);
  };

  outputFile.close();
}
} // namespace LB