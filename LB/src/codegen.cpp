#include "LA.h"
#include "parser.h"
#include "parsing/primitives.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <stack>
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
template <typename Gensym, typename Gensym2>
std::vector<LA::i> compile_LB_scope(LB::scope &scope, Gensym gensym, Gensym2 gensym2, std::vector<std::unordered_map<std::string, std::string>> &kont, WhileInfo &while_loops);
template <typename Gensym2>
std::vector<LA::i> translate_while_loop(LB::While &while_instr, Gensym2 gensym2, WhileInfo &while_loops) {
  L3::label condlabel = L3::label{":" + gensym2()};
  std::string gensymmed_var_temp = gensym2();
  while_loops.begin_while[while_instr] = while_instr.then;
  while_loops.end_while[while_instr] = while_instr.els;
  while_loops.cond_labels[while_instr] = condlabel;
  //   Identify entry and exit point of each while instruction w
  // while (%v1 = 3) :body_w :after_w
  // • beginWhile[w] = :body_w
  // • endWhile[w] = :after_w
  // 2. Add a new label l just before a while instruction w
  // • condLabels[w] = l
  return {
      L3::LabelInst{L3::label{condlabel}},
      LA::TypeVar{IR::t_int64{0}, gensymmed_var_temp},
      LA::BinOp{gensymmed_var_temp, while_instr.cnd.left, while_instr.cnd.op, while_instr.cnd.right},
      LA::BrCond{gensymmed_var_temp, while_instr.then.name, while_instr.els.name}};
};

template <typename Gensym, typename Gensym2>
static std::vector<LA::i> LB_to_LAinst(LB::i &inst, Gensym gensym, Gensym2 gensym2, std::vector<std::unordered_map<std::string, std::string>> &kont, WhileInfo &while_loops) {
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
    } else if constexpr (std::is_same_v<T, LB::ifthen>) {
      std::string gensymmed_var = gensym2();
      return {
          LA::TypeVar{IR::t_int64{0}, gensymmed_var}, LA::BinOp{gensymmed_var, ins.cnd.left, ins.cnd.op, ins.cnd.right},
          LA::BrCond{gensymmed_var, ins.then.name, ins.els.name}};
    } else if constexpr (std::is_same_v<T, LB::While>) {
      return translate_while_loop(ins, gensym2, while_loops);
    } else if constexpr (std::is_same_v<T, std::shared_ptr<LB::scope>>) {
      return compile_LB_scope(*ins, gensym, gensym2, kont, while_loops);
    } // flatten the scopes by recursively calling compile_LB_scope. Finally some proper PL stuff....
    else if constexpr (std::is_same_v<T, LB::Continue>) {
      return {LA::LB::Continue{}};
    } else if constexpr (std::is_same_v<T, LB::Break>) {
      return {LA::LB::Break{}};
    } else {
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
      for (auto &arg : ins.args)
        new_args.push_back(kont_lookup_t(kont, arg));
      return LA::CallArbitrary{kont_lookup(kont, ins.callee), new_args};
    } else if constexpr (std::is_same_v<T, LA::CallAssignArbitrary>) {
      std::vector<LA::t> new_args;
      for (auto &arg : ins.callexpr.args)
        new_args.push_back(kont_lookup_t(kont, arg));
      return LA::CallAssignArbitrary{kont_lookup(kont, ins.dest), LA::CallArbitrary{kont_lookup(kont, ins.callexpr.callee), new_args}};
    } else if constexpr (std::is_same_v<T, LA::ArrayRead>) {
      std::vector<LA::t> new_indices;
      for (auto &idx : ins.indices)
        new_indices.push_back(kont_lookup_t(kont, idx));
      return LA::ArrayRead{kont_lookup(kont, ins.dest), kont_lookup(kont, ins.src), new_indices, ins.line};
    } else if constexpr (std::is_same_v<T, LA::ArrayWrite>) {
      std::vector<LA::t> new_indices;
      for (auto &idx : ins.indices)
        new_indices.push_back(kont_lookup_t(kont, idx));
      return LA::ArrayWrite{kont_lookup(kont, ins.dest), new_indices, kont_lookup_t(kont, ins.src), ins.line};
    } else if constexpr (std::is_same_v<T, LA::Length>) {
      if (ins.dim.has_value()) {
        LA::t dim_val = ins.dim.value();
        return LA::Length{kont_lookup(kont, ins.dest), kont_lookup(kont, ins.src), kont_lookup_t(kont, dim_val)};
      } else {
        return LA::Length{kont_lookup(kont, ins.dest), kont_lookup(kont, ins.src), ins.dim};
      }
    } else if constexpr (std::is_same_v<T, LA::NewArray>) {
      std::vector<LA::t> new_dims;
      for (auto &dim : ins.dims)
        new_dims.push_back(kont_lookup_t(kont, dim));
      return LA::NewArray{kont_lookup(kont, ins.dest), new_dims};
    } else if constexpr (std::is_same_v<T, LA::NewTuple>)
      return LA::NewTuple{kont_lookup(kont, ins.dest), kont_lookup_t(kont, ins.size)};
    else if constexpr (std::is_same_v<T, LA::BrCond>)
      return LA::BrCond{kont_lookup_t(kont, ins.cond), kont_lookup(kont, ins.true_label), kont_lookup(kont, ins.false_label)};
    else
      return ins;
  },
                    compiled_instr);
};
template <typename Gensym, typename Gensym2>
std::vector<LA::i> compile_LB_scope(LB::scope &scope, Gensym gensym, Gensym2 gensym2, std::vector<std::unordered_map<std::string, std::string>> &kont, WhileInfo &while_loops) {
  std::unordered_map<std::string, std::string> map;
  kont.push_back(map);
  std::vector<LA::i> instructions;
  for (auto &instr : scope.instructions) {
    auto compiled_insts = LB_to_LAinst(instr, gensym, gensym2, kont, while_loops); // pass the gensym function, the kontinuation of the environments and the instruction

    for (auto &compiled_instr : compiled_insts) {
      instructions.push_back(refactor_old_vars(compiled_instr, kont));
    }
  };
  kont.pop_back(); // drop the map from the kontinuation when exiting the scope
  return instructions;
}
void map_loop_instrs(std::vector<LA::i> &loop_instructions, WhileInfo &while_loops) {

  std::stack<const LB::While *> loopStack;
  for (int i = 0; i < loop_instructions.size(); i++) {
    if (loopStack.size() > 0) {
      while_loops.loop[i] = loopStack.top();
      // map instructions to their more nested loop w
    }

    if (inst_is_labelinst(loop_instructions[i])) {
      L3::label label_of_interest = std::get<L3::LabelInst>(loop_instructions[i]).lbl;
      // if (i is the beginning of a while loop w
      bool do_end = true;
      for (auto &[w, lbl] : while_loops.begin_while) { // iterate through all the kep value pairs
        if (lbl.name == label_of_interest.name) {
          loopStack.push(&w); // push while loop address
          do_end = false;
        }
      }

      // else if (i is the end of a loop) {
      if (do_end) {
        for (auto &[w, lbl] : while_loops.end_while) {
          if (lbl.name == label_of_interest.name && loopStack.size() > 0) {
            loopStack.pop();
          }
        }
      }
    }
  }
}
const LB::While *fetch_innermost_loop(WhileInfo &while_loops, int idx) {
  if (while_loops.loop.count(idx)) { // check instruction is inside while loop
    return while_loops.loop[idx];
  }
  return nullptr;
}

std::vector<LA::i> translate_break_and_continue(std::vector<LA::i> &loop_instructions, WhileInfo &while_loops) {
  for (int idx = 0; idx < loop_instructions.size(); idx++) {
    loop_instructions[idx] = std::visit([&](auto &ins)
                                            -> LA::i {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, LA::LB::Continue>) {
        const LB::While *innermost_loop = fetch_innermost_loop(while_loops, idx); // Fetch the innermost loop w that i belongs to: w = Loop[i]

        L3::label cond_label = while_loops.cond_labels.at(*innermost_loop); //         • Fetch the label placed just before the condition of w:
        return L3::brlabel{cond_label};                                     // • Generate a jump to the condition code of w:

      }

      else if constexpr (std::is_same_v<T, LA::LB::Break>) {
        const LB::While *innermost_loop = fetch_innermost_loop(while_loops, idx); // Fetch the innermost loop w that i belongs to: w = Loop[i]
        L3::label exit_label = while_loops.end_while.at(*innermost_loop);         // Fetch the exit label of w: l_exit = endWhile[w]
        return L3::brlabel{exit_label};                                           // • Generate a jump to leave the loop w: br l_exit
      } else
        return ins;
    },
                                        loop_instructions[idx]);
  }
  return loop_instructions;
}

LA::fLA fLB_to_fLA(LB::fLB &f) {
  std::vector<LA::i> new_instructions;
  int counter = 0;
  int counter2 = 0;
  auto gensym = [&counter](const std::string &name) -> std::string {
    return name + "_" + std::to_string(counter++);
  };
  auto gensym2 = [&counter2]() -> std::string {
    return "random_random" + std::to_string(counter2++);
  };
  std::vector<std::unordered_map<std::string, std::string>> kont;
  WhileInfo while_loops;
  new_instructions = compile_LB_scope(*f.lscope, gensym, gensym2, kont, while_loops);
  map_loop_instrs(new_instructions, while_loops);
  new_instructions = translate_break_and_continue(new_instructions, while_loops);
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