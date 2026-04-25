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
void interference_for_row(std::vector<w> &array_row, const w &variable_of_interest, std::vector<std::vector<w>> &IN,
                          std::vector<std::vector<w>> &OUT, std::vector<std::vector<w>> &KILL) {
  array_row.push_back(variable_of_interest);
  // if the variable of interest if a register, then we begin with a fully connected graph for the registers
  // else we begin with an empty graph
  std::set<w> nodes;
  if (std::holds_alternative<r>(variable_of_interest)) {
    nodes.insert(gp_registers.begin(), gp_registers.end());
  }
  // belong in the same out set
  belong_same_out_in_set(nodes, OUT, variable_of_interest);
  // belong in the same in set
  belong_same_out_in_set(nodes, IN, variable_of_interest);
  // variables in kill with those in OUT
  kill_out_connect(nodes, OUT, KILL, variable_of_interest);
  for (const auto &node : nodes) {
    if (variable_of_interest != node) {
      array_row.push_back(node);
    }
  };
};
template <typename T>
void add_edge(std::vector<std::vector<T>> &array, const T &a, const T &b) {
  for (auto &row : array) {
    if (row.empty())
      continue; // since the array is presized, this prevents accessing elements from empty rows...
    // add registers to the interference row of the shift position of sopreg.
    if (row[0] == b && std::find(row.begin(), row.end(), a) == row.end()) {
      row.push_back(a);
    }
    // add edges between b and every register
    else if (row[0] == a && std::find(row.begin(), row.end(), b) == row.end()) {
      row.push_back(b);
    }
  }
};
void sopreg_constraint(std::vector<std::vector<w>> &interference_array, std::vector<i> &instructions) {
  for (auto &instr : instructions) {
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, SopReg>) { // filter for the sopreg instruction
        std::visit([&](auto &v) {
          using V = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<V, r> || std::is_same_v<V, var>) { // only if the shift operand is either a register or a var
            w shift_var = w{v};
            for (auto &reg : gp_registers) {
              if (std::get<r>(reg) != r::rcx) {
                // if the variable of interest in the row is equal to the one in the shift position of sopreg
                // add an edge to all registers except rcx, and ensure that the register isn't already there...
                add_edge(interference_array, reg, shift_var);
              }
            }
          }
        },
                   ins.shift);
      }
    },
               instr);
  }
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
  for (int i = 0; i < num_unique_variables; i++) {
    interference_for_row(interference_array[i], nodes_vec[i], IN, OUT, KILL);
  }
  sopreg_constraint(interference_array, function.instructions);

  return interference{interference_array};
};
void print_interference(interference result) {
  graph_to_cout(result.array);
};

} // namespace L2