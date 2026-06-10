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
template <typename Gensym>
std::vector<LA::i> compile_LB_scope(LB::scope &scope, Gensym gensym, std::vector<std::unordered_map<std::string, std::string>> &kont);
template <typename Gensym>
static std::vector<LA::i> LB_to_LAinst(LB::i &inst, Gensym gensym, std::vector<std::unordered_map<std::string, std::string>> &kont) {
  return std::visit([&](auto &ins) -> std::vector<LA::i> {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_constructible_v<LA::i, T>) {
      return {ins}; // expressions which are passthrough, in the LA namespace...
    } else if constexpr (std::is_same_v<T, LB::ReturnTOpt>) {
      if (ins.return_thing.has_value()) {
        return {LA::ReturnT{ins.return_thing.value()}};

      } else {
        return {L3::Return{}};
      }
    } else if constexpr (std::is_same_v<T, LB::TypeVars>) {
      std::vector<LA::i> compiled_var_decls;
      for (auto variable : ins.vars_declared) {
        std::string gensymmed_var = gensym(variable);                           // rename v in i to a new and unique name z
        kont.back()[variable] = gensymmed_var;                                  // remember the mapping v -> z, for the particular scope
        compiled_var_decls.push_back(LA::TypeVar{ins.var_type, gensymmed_var}); // return the gensymmed variable for this definiton
      }
      return compiled_var_decls;
    } else if constexpr (std::is_same_v<T, L3::Goto>) {
      return {
          L3::brlabel{ins.target}};
    } else if constexpr (std::is_same_v<T, std::shared_ptr<LB::scope>>) {
      return compile_LB_scope(*ins, gensym, kont);
    } // flatten the scopes by recursively calling compile_LB_scope. Finally some proper PL stuff....

    else {
      return {L3::Return{}}; // TODO remove this placeholder...
    }
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

template <typename T, typename F> // why is there no map in cpp???? .......
auto map(const std::vector<T> &vec, F f) {
  std::vector<decltype(f(vec[0]))> result;
  result.reserve(vec.size());
  for (const auto &x : vec) {
    result.push_back(f(x));
  }
  return result;
}

std::string kont_lookup(std::vector<std::unordered_map<std::string, std::string>> &kont, std::string &var) {

  for (int i = kont.size() - 1; i >= 0; i--) {
    if (kont[i].count(var)) {
      return kont[i][var]; // iterate through the scopes in reverse order, to get the mapping from the deepest scope first
    }
  }
  return var; // O.W just return the original variable
};

LA::t kont_lookup_t(std::vector<std::unordered_map<std::string, std::string>> &kont, LA::t &possible_var) {
  return std::visit([&](auto &possible_var) -> LA::t {
    using T = std::decay_t<decltype(possible_var)>;
    if constexpr (std::is_same_v<T, L3::N>)
      return possible_var; // if the variable is a number, just return t
    else if constexpr (std::is_same_v<T, std::string>)
      return kont_lookup(kont, possible_var); // O.W. perform the lookup in the kont
  },
                    possible_var);
};

LA::i refactor_old_vars(LA::i &compiled_instr, std::vector<std::unordered_map<std::string, std::string>> &kont) {
  return std::visit([&](auto &ins) -> LA::i {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, LA::Assign>)
        return LA::Assign{kont_lookup(kont, ins.dest), kont_lookup_t(kont, ins.src)};
      else if constexpr (std::is_same_v<T, LA::BinOp>)
        return LA::BinOp{kont_lookup(kont, ins.dest), kont_lookup_t(kont, ins.left), ins.operation, kont_lookup_t(kont, ins.right)};
      else if constexpr (std::is_same_v<T, LA::CallArbitrary>) {
        std::vector<LA::t> new_args;
        for (auto &arg : ins.args) new_args.push_back(kont_lookup_t(kont, arg));
        return LA::CallArbitrary{kont_lookup(kont, ins.callee), new_args};
      }
      else if constexpr (std::is_same_v<T, LA::CallAssignArbitrary>) {
        std::vector<LA::t> new_args;
        for (auto &arg : ins.callexpr.args) new_args.push_back(kont_lookup_t(kont, arg));
        return LA::CallAssignArbitrary{kont_lookup(kont, ins.dest), LA::CallArbitrary{kont_lookup(kont, ins.callexpr.callee), new_args}};
      }
      else if constexpr (std::is_same_v<T, LA::ArrayRead>) {
        std::vector<LA::t> new_indices;
        for (auto &idx : ins.indices) new_indices.push_back(kont_lookup_t(kont, idx));
        return LA::ArrayRead{kont_lookup(kont, ins.dest), kont_lookup(kont, ins.src), new_indices, ins.line};
      }
      else if constexpr (std::is_same_v<T, LA::ArrayWrite>) {
        std::vector<LA::t> new_indices;
        for (auto &idx : ins.indices) new_indices.push_back(kont_lookup_t(kont, idx));
        return LA::ArrayWrite{kont_lookup(kont, ins.dest), new_indices, kont_lookup_t(kont, ins.src), ins.line};
      }
      else if constexpr (std::is_same_v<T, LA::Length>) {
        if (ins.dim.has_value()) {
          LA::t dim_val = ins.dim.value();
          return LA::Length{kont_lookup(kont, ins.dest), kont_lookup(kont, ins.src), kont_lookup_t(kont, dim_val)};
        } else {
          return LA::Length{kont_lookup(kont, ins.dest), kont_lookup(kont, ins.src), ins.dim};
        }
      }
      else if constexpr (std::is_same_v<T, LA::NewArray>) {
        std::vector<LA::t> new_dims;
        for (auto &dim : ins.dims) new_dims.push_back(kont_lookup_t(kont, dim));
        return LA::NewArray{kont_lookup(kont, ins.dest), new_dims};
      }
      else if constexpr (std::is_same_v<T, LA::NewTuple>)
        return LA::NewTuple{kont_lookup(kont, ins.dest), kont_lookup_t(kont, ins.size)};
      else if constexpr (std::is_same_v<T, LA::BrCond>)
        return LA::BrCond{kont_lookup_t(kont, ins.cond), kont_lookup(kont, ins.true_label), kont_lookup(kont, ins.false_label)};
      else if constexpr (std::is_same_v<T, LA::TypeVar>)
        return ins;
      else if constexpr (std::is_same_v<T, L3::Return>)
        return ins;
      else if constexpr (std::is_same_v<T, LA::ReturnT>)
        return ins;
      else if constexpr (std::is_same_v<T, L3::LabelInst>)
        return ins;
      else if constexpr (std::is_same_v<T, L3::brlabel>)
        return ins; },
                    compiled_instr);
};
template <typename Gensym>
std::vector<LA::i> compile_LB_scope(LB::scope &scope, Gensym gensym, std::vector<std::unordered_map<std::string, std::string>> &kont) {
  std::unordered_map<std::string, std::string> map;
  kont.push_back(map);
  std::vector<LA::i> instructions;
  for (auto &instr : scope.instructions) {
    auto compiled_insts = LB_to_LAinst(instr, gensym, kont); // pass the gensym function, the kontinuation of the environments and the instruction

    for (auto &compiled_instr : compiled_insts) {
      instructions.push_back(refactor_old_vars(compiled_instr, kont));
    }
  };
  kont.pop_back(); // drop the map from the kontinuation when exiting the scope
  return instructions;
}

LA::fLA fLB_to_fLA(LB::fLB &f) {
  std::vector<LA::i> new_instructions;
  int counter = 0;
  auto gensym = [&counter](const std::string &name) -> std::string {
    return name + "_" + std::to_string(counter++);
  };
  std::vector<std::unordered_map<std::string, std::string>> kont;
  new_instructions = compile_LB_scope(*f.lscope, gensym, kont);
  LA::fLA result;
  result.name = f.name;
  result.f_type = f.f_type;
  result.parameters = f.parameters;
  result.instructions = new_instructions;
  return result;
}

LA::pLA pLB_to_pLA(LB::pLB &p) {
  std::vector<LA::fLA> function_list;

  for (auto &fn : p.functions) {
    function_list.push_back(fLB_to_fLA(fn));
  }
  return LA::pLA{
      function_list};
};

void generate_code(pLB &p) {
  LA::pLA transformed_p;
  transformed_p = pLB_to_pLA(p);
  std::ofstream outputFile;
  outputFile.open("prog.LA");

  // // build the LA code for each of the funtions inside of the ast/program
  for (auto &fn : transformed_p.functions) {
    print_functionLA(fn, true, &outputFile);
  };

  outputFile.close();
}
} // namespace LB