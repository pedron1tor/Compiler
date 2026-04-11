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

void instruction_to_assembly(std::ofstream &outputfile, i &instruction) {
  std::visit([&](auto &inst) {
    using T = std::decay_t<decltype(inst)>;
    if constexpr (std::is_same_v<T, Return>) {
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
  for (auto &instruction : function.instructions) {
    instruction_to_assembly(outputfile, instruction);
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