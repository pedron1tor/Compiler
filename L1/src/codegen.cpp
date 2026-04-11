#include "L1.h"
#include "parser.h"
#include <fstream>

namespace L1 {
void push_callee_saved_registers(std::ofstream &outputfile) {
  outputfile << "\tpushq %rbx\n";
  outputfile << "\tpushq %rbp\n";
  outputfile << "\tpushq %r12\n";
  outputfile << "\tpushq %r13\n";
  outputfile << "\tpushq %r14\n";
  outputfile << "\tpushq %r15\n";
};
void retq(std::ofstream &outputfile) {
  outputfile << "\tretq\n";
};
void pop_callee_saved_registers(std::ofstream &outputfile) {
  outputfile << "\tpopq %r15\n";
  outputfile << "\tpopq %r14\n";
  outputfile << "\tpopq %r13\n";
  outputfile << "\tpopq %r12\n";
  outputfile << "\tpopq %rbp\n";
  outputfile << "\tpopq %rbx\n";
  retq(outputfile);
}
std::string rename_f(std::string at_label) {
  at_label[0] = '_';
  return at_label;
};
std::string rename_register(std::string reg) {
  reg = "%" + reg;
  return reg;
};
std::string rename_constant(int64_t number) {
  std::string num_string = "$" + std::to_string(number);
  return num_string;
};
std::string s_set_to_assembly(L1::s val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, L1::r>)
      return "%" + r_to_str(v);
    else if constexpr (std::is_same_v<T, L1::N>)
      return "$" + std::to_string(v.value);
    else if constexpr (std::is_same_v<T, L1::label>)
      return "$_" + v.name.substr(1); // remove leading ':'
    else
      return "$_" + v.name.substr(1); // remove leading '@'
  },
                    val);
}
std::string t_set_to_assembly(L1::t val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, L1::r>)
      return "%" + r_to_str(v);
    else if constexpr (std::is_same_v<T, L1::N>)
      return "$" + std::to_string(v.value);
  },
                    val);
}

void arithmetic_operation(std::ofstream &outputfile, AopInst arithmetic_instruction) {
  std::string operation = aop_to_str(arithmetic_instruction.op);
  if (operation == "+=") {
    outputfile << "\taddq " << t_set_to_assembly(arithmetic_instruction.src) << ", " << r_to_str(arithmetic_instruction.dest) << "\n";
  } else if (operation == "-=") {
    outputfile << "\tsubq " << t_set_to_assembly(arithmetic_instruction.src) << ", " << r_to_str(arithmetic_instruction.dest) << "\n";
  } else if (operation == "*=") {
    outputfile << "\tmultq " << t_set_to_assembly(arithmetic_instruction.src) << ", " << r_to_str(arithmetic_instruction.dest) << "\n";
  } else if (operation == "&=") {
    outputfile << "\tandq " << t_set_to_assembly(arithmetic_instruction.src) << ", " << r_to_str(arithmetic_instruction.dest) << "\n";
  }
};
std::string mem_format_offset(std::string reg, int64_t offset) {
  reg = "(" + reg + ")";
  reg = std::to_string(offset) + reg;
  return reg;
};
void mem_arithmetic_operation_read(std::ofstream &outputfile, MemAopRead instruction) {
  std::string operation = aop_to_str(instruction.operation);
  if (operation == "-=") {
    outputfile << "\tsubq " << mem_format_offset(rename_register(r_to_str(instruction.base)), instruction.offset) << ", " << rename_register(r_to_str(instruction.dest)) << "\n";
  } else if (operation == "+=") {
    outputfile << "\taddq " << mem_format_offset(rename_register(r_to_str(instruction.base)), instruction.offset) << ", " << rename_register(r_to_str(instruction.dest)) << "\n";
  }
};
void mem_arithmetic_operation_write(std::ofstream &outputfile, MemAopWrite instruction) {
  std::string operation = aop_to_str(instruction.operation);
  if (operation == "-=") {
    outputfile << "\tsubq " << t_set_to_assembly(instruction.src) << ", " << mem_format_offset(rename_register(r_to_str(instruction.base)), instruction.offset) << "\n";
  } else if (operation == "+=") {
    outputfile << "\taddq " << t_set_to_assembly(instruction.src) << ", " << mem_format_offset(rename_register(r_to_str(instruction.base)), instruction.offset) << "\n";
  }
}
std::string register_to_8bit_variant(std::string reg) {
  if (reg == "r10")
    return "r10b";
  if (reg == "r11")
    return "r11b";
  if (reg == "r12")
    return "r12b";
  if (reg == "r13")
    return "r13b";
  if (reg == "r14")
    return "r14b";
  if (reg == "r15")
    return "r15b";
  if (reg == "r8")
    return "r8b";
  if (reg == "r9")
    return "r9b";
  if (reg == "rax")
    return "al";
  if (reg == "rbp")
    return "bpl";
  if (reg == "rbx")
    return "bl";
  if (reg == "rcx")
    return "cl";
  if (reg == "rdi")
    return "dil";
  if (reg == "rdx")
    return "dl";
  if (reg == "rsi")
    return "sil";
  return reg;
}
int cmp_result(std::string cmp_op, int64_t lv, int64_t rv) {
  int result;
  if (cmp_op == "<")
    result = (lv < rv) ? 1 : 0;
  else if (cmp_op == "<=")
    result = (lv <= rv) ? 1 : 0;
  else
    result = (lv == rv) ? 1 : 0;
  return result;
};
void comparisons_to_assembly(std::ofstream &outputfile, Compare instruction) {
  std::string cmp_op = cmp_to_str(instruction.op);
  bool left_is_num = std::holds_alternative<N>(instruction.left);
  bool right_is_num = std::holds_alternative<N>(instruction.right);
  std::string small_register = register_to_8bit_variant(r_to_str(instruction.dest));
  std::string dest_reg = rename_register(r_to_str(instruction.dest));

  if (left_is_num && right_is_num) {
    // compute the comparison in compile time
    int64_t lv = std::get<N>(instruction.left).value;
    int64_t rv = std::get<N>(instruction.right).value;
    int result = cmp_result(cmp_op, lv, rv);
    outputfile << "\tmovq $" << result << ", " << dest_reg << "\n";
  } else if (left_is_num) {
    outputfile << "\tcmpq " << t_set_to_assembly(instruction.left) << ", " << t_set_to_assembly(instruction.right) << "\n";
    if (cmp_op == "<")
      outputfile << "\tsetg " << rename_register(small_register) << "\n";
    else if (cmp_op == "<=")
      outputfile << "\tsetge " << rename_register(small_register) << "\n";
    else
      outputfile << "\tsete " << rename_register(small_register) << "\n";
    outputfile << "\tmovzbq " << rename_register(small_register) << ", " << dest_reg << "\n";
  } else {
    // left is reg, right may be reg or num
    outputfile << "\tcmpq " << t_set_to_assembly(instruction.right) << ", " << t_set_to_assembly(instruction.left) << "\n";
    if (cmp_op == "<")
      outputfile << "\tsetl " << rename_register(small_register) << "\n";
    else if (cmp_op == "<=")
      outputfile << "\tsetle " << rename_register(small_register) << "\n";
    else
      outputfile << "\tsete " << rename_register(small_register) << "\n";
    outputfile << "\tmovzbq " << rename_register(small_register) << ", " << dest_reg << "\n";
  }
}
template <typename SopT>
void shift_ops_to_assembly(std::ofstream &outputfile, SopT &instruction) {
  std::string left = rename_register(r_to_str(instruction.dest));
  std::string assembly_op = (instruction.op == sop::lshift) ? "\tsalq " : "\tsarq ";
  std::string right;
  if constexpr (std::is_same_v<SopT, SopReg>) {
    right = rename_register(register_to_8bit_variant(r_to_str(instruction.shift)));
  } else {
    right = rename_constant(instruction.shift.value);
  }
  outputfile << assembly_op << right << ", " << left << "\n";
}
void conditional_jumps_to_assembly(std::ofstream &outputfile, Cjump &instruction) {
  bool left_is_num = std::holds_alternative<N>(instruction.left);
  bool right_is_num = std::holds_alternative<N>(instruction.right);
  std::string cmp_op = cmp_to_str(instruction.op);
  std::string assembly_target = rename_f(instruction.target.name);
  if (left_is_num && right_is_num) {
    int64_t lv = std::get<N>(instruction.left).value;
    int64_t rv = std::get<N>(instruction.right).value;
    int result = cmp_result(cmp_op, lv, rv);
    if (result) {
      outputfile << "\tjmp " << assembly_target << "\n";
    }
  } else if (left_is_num) {
    // swap operands and invert jump due to assembly cmp requirements
    outputfile << "\tcmpq " << t_set_to_assembly(instruction.left) << ", " << t_set_to_assembly(instruction.right) << "\n";
    if (cmp_op == "<=")
      outputfile << "\tjge " << assembly_target << "\n";
    else if (cmp_op == "<")
      outputfile << "\tjg " << assembly_target << "\n";
    else
      outputfile << "\tje " << assembly_target << "\n";
  } else {
    outputfile << "\tcmpq " << t_set_to_assembly(instruction.right) << ", " << t_set_to_assembly(instruction.left) << "\n";
    if (cmp_op == "<=")
      outputfile << "\tjle " << assembly_target << "\n";
    else if (cmp_op == "<")
      outputfile << "\tjl " << assembly_target << "\n";
    else
      outputfile << "\tje " << assembly_target << "\n";
  }
}
void instruction_to_assembly(std::ofstream &outputfile, i &instruction, int num_locals) {
  std::visit([&](auto &inst) {
    using T = std::decay_t<decltype(inst)>;
    if constexpr (std::is_same_v<T, Return>) {
      int subq_num = num_locals*8; 
      if (num_locals > 0){
      outputfile << "\taddq " << rename_constant(subq_num) << ", " << rename_register("rsp") << "\n";
      };
      retq(outputfile);
    } else if constexpr (std::is_same_v<T, Assign>) {
      outputfile << "\tmovq " << s_set_to_assembly(inst.src) << ", " << rename_register(r_to_str(inst.dest)) << "\n";
    } else if constexpr (std::is_same_v<T, MemRead>) {
      outputfile << "\tmovq " << mem_format_offset(r_to_str(inst.base), inst.offset) << ", " << rename_register(r_to_str(inst.dest)) << "\n";
    } else if constexpr (std::is_same_v<T, MemWrite>) {
      outputfile << "\tmovq " << s_set_to_assembly(inst.src) << ", " << mem_format_offset(rename_register(r_to_str(inst.base)), inst.offset) << "\n";
    } else if constexpr (std::is_same_v<T, AopInst>) {
      arithmetic_operation(outputfile, inst);
    } else if constexpr (std::is_same_v<T, Increment>) {
      outputfile << "\tinc " << rename_register(r_to_str(inst.reg)) << "\n";
    } else if constexpr (std::is_same_v<T, Decrement>) {
      outputfile << "\tdec " << rename_register(r_to_str(inst.reg)) << "\n";
    } else if constexpr (std::is_same_v<T, MemAopRead>) {
      mem_arithmetic_operation_read(outputfile, inst);
    } else if constexpr (std::is_same_v<T, MemAopWrite>) {
      mem_arithmetic_operation_write(outputfile, inst);
    } else if constexpr (std::is_same_v<T, Compare>) {
      comparisons_to_assembly(outputfile, inst);
    } else if constexpr (std::is_same_v<T, SopReg> || std::is_same_v<T, SopNum>) {
      shift_ops_to_assembly(outputfile, inst);
    } else if constexpr (std::is_same_v<T, LabelInst>) {
      outputfile << "\t" << rename_f(inst.lbl.name) << ":" << "\n";
    } else if constexpr (std::is_same_v<T, Goto>) {
      outputfile << "\t" << "jmp " << rename_f(inst.target.name) << "\n";
    } else if constexpr (std::is_same_v<T, Cjump>) {
      conditional_jumps_to_assembly(outputfile, inst);
    } else if constexpr (std::is_same_v<T, LoadAddr>) {
        outputfile << "\tlea (" << rename_register(r_to_str(inst.base)) << ", " << rename_register(r_to_str(inst.index)) << ", " << inst.scale << "), " << rename_register(r_to_str(inst.dest)) << "\n";
    }
    // } else if constexpr (std::is_same_v<T, AopInst>) {
    //     // handle AopInst
    // } else if constexpr (std::is_same_v<T, Return>) {

    // }
  },
             instruction);
}
void function_to_assembly(std::ofstream &outputfile, f &function) {
  outputfile << rename_f(function.name.name) << ":\n";
  int subq_num = function.num_locals*8; 
  if (function.num_locals > 0){
  outputfile << "\tsubq " << rename_constant(subq_num) << ", " << rename_register("rsp") << "\n";
  };
  for (auto &instruction : function.instructions) {
    instruction_to_assembly(outputfile, instruction, function.num_locals);
  };
};

void generate_code(p &p) {

  std::ofstream outputFile;
  outputFile.open("prog.S");
  outputFile << ".text\n";
  outputFile << ".globl go\n";
  outputFile << "go:\n";
  // push the callee saved registers
  push_callee_saved_registers(outputFile);
  // call entrypoint for L1 code
  outputFile << "\tcall " << rename_f(p.entry_point.name) << "\n";
  // pop the callee saved registers
  pop_callee_saved_registers(outputFile);
  for (auto &fn : p.functions) {
    function_to_assembly(outputFile, fn);
  };

  outputFile.close();
}
} // namespace L1