#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
std::vector<w> compute_IN(std::vector<w> &GEN, std::vector<w> &KILL, std::vector<w> &OUT) {
  std::set<w> s_OUT(OUT.begin(), OUT.end());
  for (const auto &k : KILL) {
    s_OUT.erase(k);
  };
  std::set<w> s_IN(GEN.begin(), GEN.end());
  s_IN.insert(s_OUT.begin(), s_OUT.end());
  return std::vector<w>(s_IN.begin(), s_IN.end());
};
std::vector<w> compute_OUT(std::vector<std::vector<w>> &OUT, std::vector<std::vector<w>> &IN, std::vector<std::vector<int>> &successors, int inst_counter) {
  OUT[inst_counter].clear();
  std::set<w> out_set;
  for (int s : successors[inst_counter]) {
    out_set.insert(IN[s].begin(), IN[s].end());
  }
  return std::vector<w>(out_set.begin(), out_set.end());
};
void INOUT_to_cout(std::vector<std::vector<w>> &IN) {
  for (const auto &inst : IN) {
    std::cout << "(";
    bool first = true;
    for (const auto &var : inst) {
      if (!first) {
        std::cout << " ";
      }
      std::cout << w_to_str(var);
      first = false;
    };
    std::cout << ")\n";
  };
};
template <typename all_inst>
void push_to_gen(std::vector<w> &gen_inst, all_inst &ins) {
  std::visit([&](auto &v) {
    using V = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<V, r>) {
      if (v != r::rsp)
        gen_inst.push_back(w{v});
    } else if constexpr (std::is_same_v<V, var>) {
      gen_inst.push_back(w{v});
    }
  },
             ins);
};
template <typename all_inst>
void push_to_kill(std::vector<w> &gen_inst, all_inst &ins) {
  std::visit([&](auto &v) {
    using V = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<V, r>) {
      if (v != r::rsp)
        gen_inst.push_back(w{v});
    } else if constexpr (std::is_same_v<V, var>) {
      gen_inst.push_back(w{v});
    }
  },
             ins);
};

void push_args_to_gen(std::vector<w> &gen_inst, int64_t num_args) {
  for (int a = 0; a < num_args && a < (int)arg_registers.size(); a++) {
    gen_inst.push_back(arg_registers[a]);
  }
};
void push_caller_saved_r(std::vector<std::vector<w>> &KILL, int &inst_counter) {
  for (auto &r_caller : caller_saved_registers) {
    KILL[inst_counter].push_back(r_caller);
  };
};
liveness compute_liveness(f &function) {
  auto n = function.instructions.size();
  std::vector<std::vector<w>> GEN(n);
  std::vector<std::vector<w>> KILL(n);
  std::vector<std::vector<int>> successors(n);
  // do a first pass to build a map between the labelinstructions and their respective line numbers for control flow jumps
  std::map<std::string, int> label_to_index;
  for (int j = 0; j < (int)n; j++) {
    if (auto *li = std::get_if<LabelInst>(&function.instructions[j])) {
      label_to_index[li->lbl.name] = j;
    }
  }
  int inst_counter = 0;

  for (auto &instr : function.instructions) {
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, Return>) {
        GEN[inst_counter].push_back(r::rax);
        for (auto &r_callee : callee_saved_registers) {
          GEN[inst_counter].push_back(r_callee);
        };
      } else if constexpr (std::is_same_v<T, Assign>) {
        push_to_gen(GEN[inst_counter], ins.src);
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, AopInst>) {
        push_to_gen(GEN[inst_counter], ins.src);
        push_to_gen(GEN[inst_counter], ins.dest);
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, SopReg>) {
        push_to_gen(GEN[inst_counter], ins.shift);
        push_to_gen(GEN[inst_counter], ins.dest);
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, SopNum>) {
        push_to_gen(GEN[inst_counter], ins.dest);
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, LabelInst>) {
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, CallPrint>) {
        push_args_to_gen(GEN[inst_counter], 1);
        push_caller_saved_r(KILL, inst_counter);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, CallInput>) {
        push_caller_saved_r(KILL, inst_counter);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, CallAllocate>) {
        push_args_to_gen(GEN[inst_counter], 2);
        push_caller_saved_r(KILL, inst_counter);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, CallTupleError>) {
        push_args_to_gen(GEN[inst_counter], 3);
        push_caller_saved_r(KILL, inst_counter);
      } else if constexpr (std::is_same_v<T, Increment>) {
        push_to_gen(GEN[inst_counter], ins.reg);
        push_to_kill(KILL[inst_counter], ins.reg);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, Decrement>) {
        push_to_gen(GEN[inst_counter], ins.reg);
        push_to_kill(KILL[inst_counter], ins.reg);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, CallTensorError>) {
        push_args_to_gen(GEN[inst_counter], ins.F);
        push_caller_saved_r(KILL, inst_counter);
      } else if constexpr (std::is_same_v<T, Compare>) {
        push_to_kill(KILL[inst_counter], ins.dest);
        push_to_gen(GEN[inst_counter], ins.right);
        push_to_gen(GEN[inst_counter], ins.left);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, Cjump>) {
        push_to_gen(GEN[inst_counter], ins.left);
        push_to_gen(GEN[inst_counter], ins.right);
        successors[inst_counter].push_back(inst_counter + 1);
        successors[inst_counter].push_back(label_to_index[ins.target.name]);
      } else if constexpr (std::is_same_v<T, Goto>) {
        successors[inst_counter].push_back(label_to_index[ins.target.name]);
      } else if constexpr (std::is_same_v<T, MemRead>) {
        push_to_gen(GEN[inst_counter], ins.base);
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, MemWrite>) {
        push_to_gen(GEN[inst_counter], ins.src);
        push_to_gen(GEN[inst_counter], ins.base);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, MemAopRead>) {
        push_to_gen(GEN[inst_counter], ins.dest);
        push_to_gen(GEN[inst_counter], ins.base);
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, MemAopWrite>) {
        push_to_gen(GEN[inst_counter], ins.base);
        push_to_gen(GEN[inst_counter], ins.src);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, StackArg>) {
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, Call>) {
        push_caller_saved_r(KILL, inst_counter);
        push_to_gen(GEN[inst_counter], ins.callee);
        push_args_to_gen(GEN[inst_counter], ins.num_args.value);
        successors[inst_counter].push_back(inst_counter + 1);
      } else if constexpr (std::is_same_v<T, LoadAddr>) {
        push_to_gen(GEN[inst_counter], ins.base);
        push_to_gen(GEN[inst_counter], ins.index);
        push_to_kill(KILL[inst_counter], ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      }
    },
               instr);
    inst_counter++;
  };

  std::vector<std::vector<w>> IN(n);
  std::vector<std::vector<w>> OUT(n);
  bool changes = true;
  while (changes) {
    changes = false;
    for (int i = n - 1; i >= 0; i--) {
      auto prev_in = IN[i];
      auto prev_out = OUT[i];
      OUT[i] = compute_OUT(OUT, IN, successors, i);
      IN[i] = compute_IN(GEN[i], KILL[i], OUT[i]);
      if (IN[i] != prev_in || OUT[i] != prev_out) {
        changes = true;
      }
    }
  }
  return liveness{IN, OUT, KILL};
};
void print_liveness(liveness result) {
  std::cout << "(\n";
  std::cout << "(in\n";
  INOUT_to_cout(result.in);
  std::cout << ")\n\n";
  std::cout << "(out\n";
  INOUT_to_cout(result.out);
  std::cout << ")\n\n";
  std::cout << ")";
};

} // namespace L2