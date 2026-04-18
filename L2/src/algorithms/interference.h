#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
void graph_to_cout(std::vector<std::vector<w>> &interference_array) {
  for (const auto &row : interference_array) {
    bool first = true;
    for (const auto &var : row) {
      if (!first) {
        std::cout << " ";
      }
      std::cout << w_to_str(var);
      first = false;
    };
    std::cout << "\n";
  };
};
void push_gp_registers(std::vector<std::vector<w>> &array, int &row) {
  for (auto &reg_row : gp_registers) {
    array[row].push_back(reg_row);
    for (auto &reg_col : gp_registers) {
      if (reg_row != reg_col) {
        array[row].push_back(reg_col);
      }
    };
    row++;
  };
};

interference compute_interference(f &function) {
  // fully connected graph with all general purpose registers
  std::vector<std::vector<w>> interference_array(16);
  // int num_variables = 15;
  // for (i = 1; i < 15; i++) {
  //   push_caller_saved_r(interference_array, )
  // }
  int row = 0;
  push_gp_registers(interference_array, row);
  return interference{interference_array};
};
void print_interference(interference result) {
  graph_to_cout(result.array);
};

} // namespace L2