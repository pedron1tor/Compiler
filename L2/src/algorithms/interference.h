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
void belong_same_out_in_set(std::set<w> &nodes, std::vector<std::vector<w>> &IN_OUT, const w &var_of_interest) {
  for (auto &set : IN_OUT) {
    if (std::find(set.begin(), set.end(), var_of_interest) != set.end()) {
      nodes.insert(set.begin(), set.end());
    };
  };
};
void kill_out_connect(std::set<w> &nodes, std::vector<std::vector<w>> &OUT, std::vector<std::vector<w>> &KILL, const w &var_of_interest) {
  int n = OUT.size();
  for (int i = 0; i < n; i++) {
    if (std::find(OUT[i].begin(), OUT[i].end(), var_of_interest) != OUT[i].end()) {
      nodes.insert(KILL[i].begin(), KILL[i].end());
    } else if (std::find(KILL[i].begin(), KILL[i].end(), var_of_interest) != KILL[i].end()) {
      nodes.insert(OUT[i].begin(), OUT[i].end());
    };
  };
};
void push_gp_registers(std::vector<std::vector<w>> &array, int &row, std::vector<w> &unique_vars, std::vector<std::vector<w>> &IN,
                       std::vector<std::vector<w>> &OUT, std::vector<std::vector<w>> &KILL) {
  for (auto &reg_row : gp_registers) {
    array[row].push_back(reg_row);
    std::set<w> nodes(gp_registers.begin(), gp_registers.end());
    // belong in the same out set
    belong_same_out_in_set(nodes, OUT, reg_row);
    // belong in the same in set
    belong_same_out_in_set(nodes, IN, reg_row);
    // variables in kill with those in OUT
    kill_out_connect(nodes, OUT, KILL, reg_row);
    for (const auto &reg_col : nodes) {
      if (reg_row != reg_col) {
        array[row].push_back(reg_col);
      }
    };
    row++;
  };
};
void append_unique_variables(std::vector<std::vector<w>> &IN,
                             std::vector<std::vector<w>> &OUT, std::vector<std::vector<w>> &KILL, std::set<w> &nodes) {
  for (auto &set : IN)
    nodes.insert(set.begin(), set.end());
  for (auto &set : OUT)
    nodes.insert(set.begin(), set.end());
  for (auto &set : KILL)
    nodes.insert(set.begin(), set.end());
};
interference compute_interference(f &function, liveness &liveness_result) {
  // fully connected graph with all general purpose registers
  std::vector<std::vector<w>> OUT = liveness_result.out;
  std::vector<std::vector<w>> IN = liveness_result.in;
  std::vector<std::vector<w>> KILL = liveness_result.kill;
  std::set<w> nodes(gp_registers.begin(), gp_registers.end());
  append_unique_variables(IN, OUT, KILL, nodes);
  auto n = function.instructions.size();
  int num_unique_variables = nodes.size();
  std::vector<w> nodes_vec(nodes.begin(), nodes.end());
  std::vector<std::vector<w>> interference_array(num_unique_variables);
  int row = 0;
  push_gp_registers(interference_array, row, nodes_vec, IN, OUT, KILL);
  for (int i = row; i < num_unique_variables; i++) {
    interference_array[i].push_back(nodes_vec[i]);
    std::set<w> final_nodes(gp_registers.begin(), gp_registers.end());
    // belong in the same out set
    belong_same_out_in_set(final_nodes, OUT, nodes_vec[i]);
    // belong in the same in set
    belong_same_out_in_set(final_nodes, IN, nodes_vec[i]);
    // variables in kill with those in OUT
    kill_out_connect(final_nodes, OUT, KILL, nodes_vec[i]);
    for (const auto &reg_col : nodes) {
      if (nodes_vec[i] != reg_col) {
        interference_array[i].push_back(reg_col);
      }
    };
  }

  return interference{interference_array};
};
void print_interference(interference result) {
  graph_to_cout(result.array);
};

} // namespace L2