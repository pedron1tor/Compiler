// parser.h
#pragma once
#include "IR.h"

namespace IR
{
  pIR parse_file(char *filename);
  f parse_function(char *filename);
  f_spilled parse_function_spill(char *filename);
  void print_source_program(pIR program);
  void print_functionIR(f &fn);
  void print_functionl1(fl1 &fn, bool generator, std::ofstream *outputFile = nullptr);
  void print_functionl2(fl2 &fn, bool generator, std::ofstream *outputFile = nullptr);
} // namespace IR
