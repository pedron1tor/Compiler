#include "L3.h"
#include "parser.h"
#include <fstream>

namespace L3
{

  void generate_code(pl2 &p)
  {

    std::ofstream outputFile;
    outputFile.open("prog.L2");

    outputFile << "(" << p.entry_point.name << "\n";
    // build the L2 code for each of the funtions inside of the ast/program
    for (auto &fn : p.functions)
    {
      print_functionl2(fn, true, &outputFile);
    };
    outputFile << ")\n";

    outputFile.close();
  }
} // namespace L3