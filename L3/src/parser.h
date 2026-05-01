// parser.h
#pragma once
#include "L3.h"

namespace L3
{
  pl3 parse_file(char *filename);
  f parse_function(char *filename);
  f_spilled parse_function_spill(char *filename);
  std::string r_to_str(r reg);
  std::string t_to_str(t val);
  std::string s_to_str(s val);
  std::string w_to_str(w val);
  std::string aop_to_str(aop op);
  std::string cmp_to_str(cmp op);
  void print_source_program(pl3 program);
  void print_functionl3(f &fn);
  void print_functionl1(fl1 &fn, bool generator, std::ofstream *outputFile = nullptr);
  void print_functionl2(fl2 &fn, bool generator, std::ofstream *outputFile = nullptr);
} // namespace L3
