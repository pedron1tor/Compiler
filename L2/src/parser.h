// parser.h
#pragma once
#include "L1.h"

namespace L1 {
    p parse_file(char *filename);
    std::string r_to_str(L1::r reg);
    std::string t_to_str(L1::t val);
    std::string s_to_str(L1::s val);
    std::string aop_to_str(L1::aop op);
    std::string cmp_to_str(L1::cmp op);
    void print_source_program(p program);
}

