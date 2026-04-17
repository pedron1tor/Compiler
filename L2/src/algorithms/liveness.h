#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
std::vector<w> callee_saved_registers = {
    r::r12,
    r::r13,
    r::r14,
    r::r15,
    r::rbp,
    r::rbx};
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
    if constexpr (std::is_same_v<V, r> || std::is_same_v<V, var>) {
      gen_inst.push_back(w{v});
    }
  },
             ins);
};
liveness compute_liveness(f &function) {
  auto n = function.instructions.size();
  std::vector<std::vector<w>> GEN(n);
  std::vector<std::vector<w>> KILL(n);
  std::vector<std::vector<int>> successors(n);
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
        KILL[inst_counter].push_back(ins.dest);
        successors[inst_counter].push_back(inst_counter + 1);
      }
      // else if constexpr (std::is_same_v<T, AopInst>)
      //   std::cout << w_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
      // else if constexpr (std::is_same_v<T, SopReg>)
      //   std::cout << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << t_to_str(ins.shift);
      // else if constexpr (std::is_same_v<T, SopNum>)
      //   std::cout << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << ins.shift.value;
      // else if constexpr (std::is_same_v<T, LabelInst>)
      //   std::cout << ins.lbl.name;
      // else if constexpr (std::is_same_v<T, CallPrint>)
      //   std::cout << "call print 1";
      // else if constexpr (std::is_same_v<T, CallInput>)
      //   std::cout << "call input 0";
      // else if constexpr (std::is_same_v<T, CallAllocate>)
      //   std::cout << "call allocate 2";
      // else if constexpr (std::is_same_v<T, CallTupleError>)
      //   std::cout << "call tuple-error 3";
      // else if constexpr (std::is_same_v<T, Increment>)
      //   std::cout << w_to_str(ins.reg) << "++";
      // else if constexpr (std::is_same_v<T, Decrement>)
      //   std::cout << w_to_str(ins.reg) << "--";
      // else if constexpr (std::is_same_v<T, CallTensorError>)
      //   std::cout << "call tensor-error " << ins.F;
      // else if constexpr (std::is_same_v<T, Compare>)
      //   std::cout << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      // else if constexpr (std::is_same_v<T, Cjump>)
      //   std::cout << "cjump " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right) << " " << ins.target.name;
      // else if constexpr (std::is_same_v<T, Goto>)
      //   std::cout << "goto " << ins.target.name;
      // else if constexpr (std::is_same_v<T, MemRead>)
      //   std::cout << w_to_str(ins.dest) << " <- mem " << w_to_str(ins.base) << " " << ins.offset;
      // else if constexpr (std::is_same_v<T, MemWrite>)
      //   std::cout << "mem " << w_to_str(ins.base) << " " << ins.offset << " <- " << s_to_str(ins.src);
      // else if constexpr (std::is_same_v<T, MemAopRead>)
      //   std::cout << w_to_str(ins.dest) << " " << aop_to_str(ins.operation) << " mem " << w_to_str(ins.base) << " " << ins.offset;
      // else if constexpr (std::is_same_v<T, MemAopWrite>)
      //   std::cout << "mem " << w_to_str(ins.base) << " " << ins.offset << " " << aop_to_str(ins.operation) << " " << t_to_str(ins.src);
      // else if constexpr (std::is_same_v<T, StackArg>)
      //   std::cout << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      // else if constexpr (std::is_same_v<T, Call>)
      //   std::cout << "call " << std::visit([&](auto &v) -> std::string {
      //     using U = std::decay_t<decltype(v)>;
      //     if constexpr (std::is_same_v<U, r>)
      //       return r_to_str(v);
      //     else if (std::is_same_v<U, var>)
      //       return add_perc(v.name);
      //     else
      //       return v.name;
      //   },
      //                                      ins.callee)
      //             << " " << ins.num_args.value;
      // else if constexpr (std::is_same_v<T, LoadAddr>)
      //   std::cout << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale;
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
  return liveness{IN, OUT};
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