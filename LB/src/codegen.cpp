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
std::vector<LA::i> compile_LB_scope(LB::scope &scope);

static std::vector<LA::i> LB_to_LAinst(LB::i &inst) {
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
        compiled_var_decls.push_back(LA::TypeVar{ins.var_type, variable});
      }
      return compiled_var_decls;

    }

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

std::vector<LA::i> compile_LB_scope(LB::scope &scope) {
  std::vector<LA::i> instructions;
  for (auto &instr : scope.instructions) {
    auto compiled_insts = LB_to_LAinst(instr);
    instructions.insert(instructions.end(), compiled_insts.begin(), compiled_insts.end());
  }
  return instructions;
};

LA::fLA fLB_to_fLA(LB::fLB &f) {
  std::vector<LA::i> new_instructions;
  new_instructions = compile_LB_scope(*f.lscope);
  LA::fLA result;
  result.name = f.name;
  result.f_type = f.f_type;
  // result.parameters = ir_pars;
  result.instructions = new_instructions;
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