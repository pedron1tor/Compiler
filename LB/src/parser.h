// parser.h
#pragma once
#include "LB.h"
#include <fstream>

namespace LB {
pLB parse_file(char *filename);
void print_source_program(pLB program);
void print_functionLB(LB::fLB &fn, bool generator, std::ofstream *outputFile = nullptr); 
void print_functionLA(LA::fLA &fn, bool generator, std::ofstream *outputFile = nullptr);

} // namespace LA
