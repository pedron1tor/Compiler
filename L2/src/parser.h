// parser.h
#pragma once
#include "L2.h"

namespace L2 {
p parse_file(char *filename);
f parse_function(char *filename);
f_spilled parse_function_spill(char *filename);
std::string r_to_str(r reg);
std::string t_to_str(t val);
std::string s_to_str(s val);
std::string w_to_str(w val);
std::string aop_to_str(aop op);
std::string cmp_to_str(cmp op);
void print_source_program(p program);
void print_function(f &fn);
void print_functionl1(fl1 &fn);
} // namespace L2
