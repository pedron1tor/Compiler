// parser.h
#pragma once
#include "IR.h"
#include <fstream>

namespace IR
{
  pIR parse_file(char *filename);
  void print_source_program(pIR program);
  void print_functionIR(fIR &fn, bool generator, std::ofstream *outputFile = nullptr);
  void print_functionl3(fIR &fn, bool generator, std::ofstream *outputFile = nullptr);
  void print_functionl1(L3::fl1 &fn, bool generator, std::ofstream *outputFile = nullptr);
  void print_functionl2(L3::fl2 &fn, bool generator, std::ofstream *outputFile = nullptr);
} // namespace IR
