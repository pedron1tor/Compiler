#include "L1.h"
#include <fstream>

namespace L1 {
    void generate_code(p &p) { 
      
      std::ofstream outputFile;
      outputFile.open("prog.S");
      outputFile << ".text\n" ;
      
      outputFile.close();

    }
}