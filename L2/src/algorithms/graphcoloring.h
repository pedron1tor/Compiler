#pragma once

#include "interference.h"
#include <L2.h>
#include <parser.h>
#include <stack>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
std::pair<w, r> coloring_rule(w &var_to_color, r &color) {
  return std::visit([&](auto &v) -> std::pair<w, r> {
    using V = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<V, var>) {
      return std::make_pair(var_to_color, color);
    } else {
      return std::make_pair(var_to_color, v);
    }
  },
                    var_to_color);
};
std::pair<w, r> reg_coloring_rule(w &var_to_color) {
  return std::visit([&](auto &v) -> std::pair<w, r> {
    using V = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<V, r>) {
      return std::make_pair(var_to_color, v);
    } else {
      return std::make_pair(var_to_color, r{});
    }
  },
                    var_to_color);
};
void selective_add_edges(std::vector<std::vector<std::pair<w, r>>> &array, std::pair<w, r> &a,
                         std::vector<std::pair<w, r>> &added_vars, std::vector<w> &dep_nodes) {
  for (auto &added_var : added_vars) {
    bool adjacent = std::find(dep_nodes.begin(),
                              dep_nodes.end(), added_var.first) != dep_nodes.end();
    if (adjacent) {
      add_edge(array, added_var, a);
    }
  };
};

int select_color(w &unique_var, std::vector<w> &unique_var_to_dep_nodes, std::vector<std::pair<w, r>> added_vars, int current_color,
                 bool &coloring_succeeds) {
  bool invalid = true;
  // find the color different from its dependents in the already added variables,
  while (invalid) {
    invalid = false;
    for (auto &node : added_vars) {
      bool adjacent = std::find(unique_var_to_dep_nodes.begin(),
                                unique_var_to_dep_nodes.end(), node.first) != unique_var_to_dep_nodes.end();

      if (adjacent && node.second == colors[current_color]) {
        invalid = true;
        current_color++;
        if (current_color >= (int)colors.size()) {
          coloring_succeeds = false;
          return current_color;
        }
      }
    };
  };
  return current_color;
};

interference_colored assign_colors(f &function, interference interference, bool &coloring_succeds, var &var_to_spill) {
  coloring_succeds = true;

  std::vector<w> unique_variables;
  std::map<w, std::vector<w>> unique_variable_to_dependendency_nodes;
  int var_max_degree = 0;
  // collect the first row of the matrix. These are the nodes, or unique variables
  for (std::vector<w> &row : interference.array) {
    unique_variables.push_back(row[0]);
    std::vector<w> deps(row.begin() + 1, row.end());
    unique_variable_to_dependendency_nodes[row[0]] = deps;
    // select the variable with the highest degree for potential spilling
    if (var_max_degree < (int)deps.size() && is_real_var(row[0])) {
      var_max_degree = deps.size();
      var_to_spill = std::get<var>(row[0]);
    };
  };
  std::vector<std::vector<std::pair<w, r>>> colored_interference(unique_variables.size());
  // iterate through unique_variables and begin placing them back into the graph.
  int row = 0;
  std::vector<std::pair<w, r>> added_vars; // running collector of the variables that have been added to the graph.
  int current_color = 0;
  for (w &unique_var : unique_variables) {
    if (coloring_succeds) {
      std::pair<w, r> added_var;
      if (is_real_var(unique_var)) {
        current_color = select_color(unique_var, unique_variable_to_dependendency_nodes[unique_var], added_vars, current_color, coloring_succeds);
        added_var = coloring_rule(unique_var, colors[current_color]);
        colored_interference[row].push_back(added_var);
      } else {
        added_var = reg_coloring_rule(unique_var);
        colored_interference[row].push_back(added_var);
      };
      selective_add_edges(colored_interference, added_var, added_vars, unique_variable_to_dependendency_nodes[unique_var]); // add the edges from the variable,
      // to each of the already added variables if they're in the same row
      added_vars.push_back(added_var);
      row++;
    } else {
      break;
    };
  };
  return interference_colored{colored_interference};
};

f_spilled compute_coloring(f &function, interference &interference_result, bool &coloring_succeeds, int &num_locals) {
  var spilled_arg_1;
  var spilled_arg_2;
  // spilled variable prefix
  spilled_arg_2.name = "%spilled";
  assign_colors(function, interference_result, coloring_succeeds, spilled_arg_1);
  if (coloring_succeeds) {
    // set num locals
    spilled_arg_1.name = std::to_string(num_locals);
  }
  return f_spilled{
      function.name,
      function.num_args,
      function.instructions,
      spilled_arg_1,
      spilled_arg_2};
};

} // namespace L2