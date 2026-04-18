#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
template <typename V>
bool is_var(const V &val, const var &v) {
  return std::visit([&](const auto &x) -> bool {
    // if the value in the instruction is a variable, compare against variable of interest
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, var>)
      return x == v;
    else
      return false;
  },
                    val);
};
std::vector<i> transform_write(i &original_instruction) {

};
std::vector<i> transform_read(i &original_instruction) {
}

f compute_spilling(f_spilled &function) {
  int n = function.instructions.size();
  // boolean arrays to store a first pass of where the variable is being used and whether it's for read or write
  std::vector<bool> used(n, false);
  std::vector<bool> write(n, false);
  std::vector<bool> read(n, false);
  // original variable we need to spill
  var original_var = function.spilled_arg1;
  // prefix for the new variable we can use for the spilling
  var new_var = function.spilled_arg2;

  int instr_idx = 0;
  for (auto &instr : function.instructions) {
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, Assign>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.src, original_var);
      } else if constexpr (std::is_same_v<T, AopInst>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.dest, original_var) || is_var(ins.src, original_var);
      } else if constexpr (std::is_same_v<T, SopReg>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.dest, original_var) || is_var(ins.shift, original_var);
      } else if constexpr (std::is_same_v<T, SopNum>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.dest, original_var);
      } else if constexpr (std::is_same_v<T, Increment> || std::is_same_v<T, Decrement>) {
        write[instr_idx] = is_var(ins.reg, original_var);
        read[instr_idx] = is_var(ins.reg, original_var);
      } else if constexpr (std::is_same_v<T, Compare>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.left, original_var) || is_var(ins.right, original_var);
      } else if constexpr (std::is_same_v<T, Cjump>) {
        read[instr_idx] = is_var(ins.left, original_var) || is_var(ins.right, original_var);
      } else if constexpr (std::is_same_v<T, MemRead>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.base, original_var);
      } else if constexpr (std::is_same_v<T, MemWrite>) {
        read[instr_idx] = is_var(ins.base, original_var) || is_var(ins.src, original_var);
      } else if constexpr (std::is_same_v<T, MemAopRead>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.dest, original_var) || is_var(ins.base, original_var);
      } else if constexpr (std::is_same_v<T, MemAopWrite>) {
        read[instr_idx] = is_var(ins.base, original_var) || is_var(ins.src, original_var);
      } else if constexpr (std::is_same_v<T, StackArg>) {
        write[instr_idx] = is_var(ins.dest, original_var);
      } else if constexpr (std::is_same_v<T, Call>) {
        read[instr_idx] = is_var(ins.callee, original_var);
      } else if constexpr (std::is_same_v<T, LoadAddr>) {
        write[instr_idx] = is_var(ins.dest, original_var);
        read[instr_idx] = is_var(ins.base, original_var) || is_var(ins.index, original_var);
      }
      used[instr_idx] = read[instr_idx] || write[instr_idx];
    },
               instr);
    instr_idx++;
  };
  int variable_postfix = 0;
  std::vector<i> new_function_instructions;
  for (int i = 0; i < n; i++) {
    if (used[i]) {
      if (write[i]) {
        auto transformed = transform_write(function.instructions[i]);
        new_function_instructions.insert(
            new_function_instructions.end(),
            transformed.begin(),
            transformed.end());
      };
      if (read[i]) {
        auto transformed = transform_write(function.instructions[i]);
        new_function_instructions.insert(
            new_function_instructions.end(),
            transformed.begin(),
            transformed.end());
      };
      variable_postfix++;
    };
    new_function_instructions.push_back(function.instructions[i]);
  };

  return f{function.name, function.num_args, new_function_instructions};
};
void print_spilling(f result) {
  print_function(result);
};

} // namespace L2