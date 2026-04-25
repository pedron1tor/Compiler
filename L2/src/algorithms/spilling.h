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
template <typename V, typename R>
V replace_in_field(const V &val, const var &original_var, const R &replacement) {
  return is_var(val, original_var) ? V{replacement} : val;
};
// helper to rename the variables in the instructions with the variable+postfix number
template <typename R>
i transform_to_renamed_inst(const i &instr, const var &original_var, const R &replacement) {
  return std::visit([&](const auto &ins) -> i {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, Assign>)
      return Assign{replace_in_field(ins.dest, original_var, replacement), replace_in_field(ins.src, original_var, replacement)};
    else if constexpr (std::is_same_v<T, AopInst>)
      return AopInst{replace_in_field(ins.dest, original_var, replacement), ins.op, replace_in_field(ins.src, original_var, replacement)};
    else if constexpr (std::is_same_v<T, SopReg>)
      return SopReg{replace_in_field(ins.dest, original_var, replacement), ins.op, replace_in_field(ins.shift, original_var, replacement)};
    else if constexpr (std::is_same_v<T, SopNum>)
      return SopNum{replace_in_field(ins.dest, original_var, replacement), ins.op, ins.shift};
    else if constexpr (std::is_same_v<T, Increment>)
      return Increment{replace_in_field(ins.reg, original_var, replacement)};
    else if constexpr (std::is_same_v<T, Decrement>)
      return Decrement{replace_in_field(ins.reg, original_var, replacement)};
    else if constexpr (std::is_same_v<T, Compare>)
      return Compare{replace_in_field(ins.dest, original_var, replacement), replace_in_field(ins.left, original_var, replacement), ins.op, replace_in_field(ins.right, original_var, replacement)};
    else if constexpr (std::is_same_v<T, Cjump>)
      return Cjump{replace_in_field(ins.left, original_var, replacement), ins.op, replace_in_field(ins.right, original_var, replacement), ins.target};
    else if constexpr (std::is_same_v<T, MemRead>)
      return MemRead{replace_in_field(ins.dest, original_var, replacement), replace_in_field(ins.base, original_var, replacement), ins.offset};
    else if constexpr (std::is_same_v<T, MemWrite>)
      return MemWrite{replace_in_field(ins.base, original_var, replacement), ins.offset, replace_in_field(ins.src, original_var, replacement)};
    else if constexpr (std::is_same_v<T, MemAopRead>)
      return MemAopRead{replace_in_field(ins.dest, original_var, replacement), ins.operation, replace_in_field(ins.base, original_var, replacement), ins.offset};
    else if constexpr (std::is_same_v<T, MemAopWrite>)
      return MemAopWrite{replace_in_field(ins.base, original_var, replacement), ins.offset, ins.operation, replace_in_field(ins.src, original_var, replacement)};
    else if constexpr (std::is_same_v<T, StackArg>)
      return StackArg{replace_in_field(ins.dest, original_var, replacement), ins.num};
    else if constexpr (std::is_same_v<T, Call>)
      return Call{replace_in_field(ins.callee, original_var, replacement), ins.num_args};
    else if constexpr (std::is_same_v<T, LoadAddr>)
      return LoadAddr{replace_in_field(ins.dest, original_var, replacement), replace_in_field(ins.base, original_var, replacement), replace_in_field(ins.index, original_var, replacement), ins.scale};
    else
      return ins;
  },
                    instr);
}

fl1 compute_spilling(f_spilled &function, int num_locals) {
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
  // j because else I shadow the i from instructions and it explodes
  for (int j = 0; j < n; j++) {
    if (used[j]) {
      var replacement_variable{new_var.name + std::to_string(variable_postfix)};
      if (read[j]) {
        new_function_instructions.push_back(i{MemRead{replacement_variable, r::rsp, num_locals * 8}});
      }
      new_function_instructions.push_back(transform_to_renamed_inst(function.instructions[j], original_var, replacement_variable));
      if (write[j]) {
        new_function_instructions.push_back(i{MemWrite{r::rsp, num_locals * 8, replacement_variable}});
      }
      variable_postfix++;
    } else {
      new_function_instructions.push_back(function.instructions[j]);
    };
  };
  return fl1{function.name, function.num_args, num_locals, new_function_instructions};
};
void print_spilling(fl1 result) {
  print_functionl1(result, false);
};

} // namespace L2