// parser.h
#pragma once
#include "LA.h"
#include <fstream>

namespace LA {
pLA parse_file(char *filename);
void print_source_program(pLA program);
void print_functionLA(fLA &fn, bool generator, std::ofstream *outputFiel = nullptr);
void print_functionIR(IR::fIR &fn, bool generator, std::ofstream *outputFile = nullptr);
void print_functionl3(L3::fl3 &fn, bool generator, std::ofstream *outputFile = nullptr);
void print_functionl1(L3::fl1 &fn, bool generator, std::ofstream *outputFile = nullptr);
void print_functionl2(L3::fl2 &fn, bool generator, std::ofstream *outputFile = nullptr);
} // namespace LA
