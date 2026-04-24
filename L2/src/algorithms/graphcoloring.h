#pragma once

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
    };
  },
                    var_to_color);
};
std::pair<w, r> reg_coloring_rule(w &var_to_color) {
  return std::visit([&](auto &v) -> std::pair<w, r> {
    using V = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<V, r>) {
      return std::make_pair(var_to_color, v);
    };
  },
                    var_to_color);
};
void selective_add_edges() {};

int select_color(w &unique_var, std::vector<w> &unique_var_to_dep_nodes, std::vector<std::pair<w, r>> added_vars, int current_color) {
  bool changed = false;
  for (&node : added_vars) {
    if node
      in unique_var_to_dep_nodes {
        if (node.second == colors[current_color]) {
          changed = true;
        }
      }
  };
  if (!changed) {
    // things remain without clashing
    return current_color;
  } else if (changed) {
    // if there are two nodes of the same color connected by an edge.
    return current_color++;
  }
};

interference_colored assign_colors(f &function, interference interference, bool &coloring_succeds) {
  std::vector<std::vector<std::pair<w, r>>> colored_interference;
  std::vector<w> unique_variables;
  std::map<w, std::vector<w>> unique_variable_to_dependendency_nodes;
  // collect the first row of the matrix. These are the nodes, or unique variables
  for (std::vector<w> &row : interference.array) {
    unique_variables.push_back(row[0]);
    unique_variable_to_dependendency_nodes[row[0]] = std::vector<w>(row.begin() + 1, row.end());
  };
  // iterate through unique_variables and begin placing them back into the graph.
  int row = 0;
  std::vector<std::pair<w, r>> added_vars; // running collector of the variables that have been added to the graph.
  int current_color = 0;
  for (
      w &unique_var : unique_variables) {
    std::pair<w, r> added_var;
    if (is_real_var(unique_var)) {
      current_color = select_color(unique_var, unique_variable_to_dependendency_nodes[unique_var]);
      added_var = coloring_rule(unique_var, colors[current_color]);
      colored_interference[row].push_back(added_var);
    } else {
      added_var = reg_coloring_rule(unique_var);
      colored_interference[row].push_back(added_var);
    };
    selective_add_edges(colored_interference, added_var, added_vars);
    added_vars.push_back(added_var);
    row++;
  };
};

f_spilled compute_coloring(f &function, interference &interference_result, bool &coloring_succeeds) {
  int num_args = 0;
  var spilled_arg_1;
  var spilled_arg_2;
  // spilled variable prefix
  spilled_arg_2.name = "%spilled";
  assign_colors(function, interference_result, coloring_succeeds);
  if (coloring_succeeds) {
    // set num locals
    spilled_arg_1 = ;
  }
  return f_spilled{
      function.name,
      num_args,
      function.instructions,
      spilled_arg_1,
      spilled_arg_2};
};

} // namespace L2