#include "L2.h"
#include "parser.h"
#include <fstream>

namespace L2 {

void generate_code(pfl1 &p) {

  std::ofstream outputFile;
  outputFile.open("prog.L1");

  outputFile << "(" << p.entry_point.name << "\n";
  // build the L1 code for each of the funtions inside of the ast/program
  for (auto &fn : p.functions) {
    print_functionl1(fn, true, &outputFile);
  };
  outputFile << ")\n";

  outputFile.close();
}
} // namespace L2