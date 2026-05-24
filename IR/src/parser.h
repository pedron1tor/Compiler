// parser.h
#pragma once
#include "IR.h"
#include <fstream>

namespace IR
{
  pIR parse_file(char *filename);
  void print_source_program(pIR program);
  void print_functionIR(fIR &fn, bool generator, std::ofstream *outputFile = nullptr);
  L3::i BinOp_to_l3op(IR::BinOp &b);
  void print_functionl3(L3::fl3 &fn, bool generator, std::ofstream *outputFile = nullptr);
  void print_functionl1(L3::fl1 &fn, bool generator, std::ofstream *outputFile = nullptr);
  void print_functionl2(L3::fl2 &fn, bool generator, std::ofstream *outputFile = nullptr);
} // namespace IR
