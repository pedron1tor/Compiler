#include "IR.h"
#include "parser.h"
#include "parsing/primitives.h"
#include <fstream>
#include <functional>
#include <optional>
#include <set>
#include <unordered_map>

namespace IR {

// L3::fl3 fIR_to_fl2(fIR &f, std::unordered_map<label, label> &label_map, int &lbl_counter) {
//   fl2 new_function;
//   new_function.name = f.name;
//   new_function.num_args = f.vars.size();
//   // assign a register for each of the parameters
//   std::vector<i> register_assigns = args_to_regs(f.vars);
//   // append to the instructions of the function.
//   new_function.instructions.insert(new_function.instructions.end(), register_assigns.begin(), register_assigns.end());
//   // identify contexts
//   std::vector<context> contexts = identify_contexts(f.instructions);
//   // tree generation
//   std::vector<std::vector<Tree>> tree_contexts = transform_contexts_to_trees(contexts);
//   // tree merging
//   std::vector<std::vector<Tree>> tree_contexts_merged = merge_trees(tree_contexts);
//   // tiling...
//   std::vector<i> tiled = tiling(tree_contexts_merged, label_map, lbl_counter);
//   new_function.instructions.insert(new_function.instructions.end(), tiled.begin(), tiled.end());
//   return new_function;
// }
// function to get the labels that the terminal instruction of a basic function can lead to
std::vector<L3::label> successors(trace_bb &trace_basic_block) {
  std::vector<L3::label> successors_list;
  std::visit([&](auto &terminal_inst) {
    using T = std::decay_t<decltype(terminal_inst)>;
    if constexpr (std::is_same_v<T, L3::Goto>)
      successors_list.push_back(terminal_inst.target);
    else if constexpr (std::is_same_v<T, BrCond>) {
      successors_list.push_back(terminal_inst.true_label);
      successors_list.push_back(terminal_inst.false_label);
    } else { // nothing happens O.W.
    };
  },
             trace_basic_block.terminal);
  return successors_list;
};

static std::optional<bb> find_label(std::set<bb> &f_bb, const L3::label &target) {
  for (auto it = f_bb.begin(); it != f_bb.end(); ++it) {
    if (it->lbl == target) {
      bb result = *it;
      f_bb.erase(it);
      return result;
    }
  }
  return std::nullopt;
}

trace_bb select_next(std::set<bb> &f_bb, trace_bb &prev_bb, std::vector<L3::label> &succs) {
  // case where there is only one successor, you just skip the terminal instruction, O.W. optimize for less branches
  trace_bb selected_bb;
  selected_bb.mark = true;

  // if there are no successors, end loop
  // if the remaining bbs are empty, return an item with mark true, to end the loop
  if (f_bb.empty() || succs.size() == 0)
    return selected_bb;

  if (succs.size() == 1) {
    // skip the br condition
    if (auto match = find_label(f_bb, succs[0]))
      selected_bb = bb_to_trace_bb(*match);
  } else {
    // prefer the else bb of the conditional branch, because that way we can make this a conditional jump and if false it falls through
    if (auto match = find_label(f_bb, succs[1]))
      selected_bb = bb_to_trace_bb(*match);
    else if (auto match = find_label(f_bb, succs[0]))
      selected_bb = bb_to_trace_bb(*match);
  }

  return selected_bb;
};

pIR linearize_CFG(pIR &original_program) {
  pIR linearized_p;
  for (auto &fn : original_program.functions) {
    std::set<bb> f_bb(fn.blocks.begin(), fn.blocks.end()); // get all the basic blocks of the function
    int idx_into_vec;
    std::vector<trace> traces; // define a list of traces for the function
    if (!f_bb.empty()) {

      do {
        trace tr;
        bb first_bb = *f_bb.begin();
        f_bb.erase(first_bb);
        trace_bb bb = bb_to_trace_bb(first_bb); // initialize a trace basic block with mark set to false
        while (!bb.mark) {
          bb.mark = true;
          tr.blocks.push_back(bb);
          std::vector<L3::label> succs = successors(bb);
          bb = select_next(f_bb, bb, succs);
        }

        traces.push_back(tr);
      } while (0 < f_bb.size());
    };
    linearized_p.functions.push_back(fIR{fn.f_type, fn.name, fn.parameters, traces_to_bbs(traces)});
  };
  return linearized_p;
};
// L3::p pIR_to_pL3(pIR &p) {
//   std::string entry_point = "@main"; // main entry point
//   std::vector<L3::fl3> function_list;
//   int glob_lbl_counter = 0;
//   // hash map, IR-labels -> l2-labels
//   std::unordered_map<L3::label, L3::label> label_map;
//   for (auto &fn : p.functions) {
//     function_list.push_back(fIR_to_fl3(fn, label_map, glob_lbl_counter));
//     glob_lbl_counter++;
//   }
//   return pl2{
//       entry_point, function_list};
// };

void generate_code(pIR &p) {
  pIR linearized_p;
  // linearize CFG
  linearized_p = linearize_CFG(p);
  L3::p transformed_p;
  // transformed_p = pIR_to_pL3(linearized_p);
  // std::ofstream outputFile;
  // outputFile.open("prog.L2");

  // outputFile << "(" << transformed_p.entry_point.name << "\n";
  // // build the L2 code for each of the funtions inside of the ast/program
  // for (auto &fn : transformed_p.functions) {
  //   print_functionl2(fn, true, &outputFile);
  // };
  // outputFile << ")\n";

  // outputFile.close();
}
} // namespace IR