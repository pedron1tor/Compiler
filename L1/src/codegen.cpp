#include "L1.h"
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
      outputfile << "\tpushq %r15\n";
      outputfile << "\tpushq %r14\n";
      outputfile << "\tpushq %r13\n";
      outputfile << "\tpushq %r12\n";
      outputfile << "\tpushq %rbp\n";
      outputfile << "\tpushq %rbx\n";
      retq(outputfile);
    }
    std::string rename_f( std::string at_label) {
      at_label[0] = '_';
      return at_label;
    }
    void function_to_assembly(std::ofstream &outputfile, f &function) {
      outputfile <<rename_f(function.name.name) << ":\n";

    };
    
    
    void generate_code(p &p) { 
      
      std::ofstream outputFile;
      outputFile.open("prog.S");
      outputFile << ".text\n" ;
      outputFile << ".globl go\n" ;
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
}