#include "IR.h"
#include "parser.h"
#include "parsing/primitives.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_map>
#include <utils.h>

namespace IR {

L3::fl3 fIR_to_fl3(fIR &f) {
  L3::fl3 new_function;
  new_function.name = f.name;
  for (auto &tv : f.parameters.input_variables) // unwrap the variable names from the type tuples
    new_function.vars.push_back(tv.var_declared);

  for (int block_idx = 0; block_idx < f.blocks.size(); block_idx++) {
    auto &block = f.blocks[block_idx];
    new_function.instructions.push_back(L3::LabelInst{block.lbl});
    // pattern match the instructions first
    for (auto &instr : block.instructions) {
      std::visit([&](auto &inst) {
        using T = std::decay_t<decltype(inst)>;
        if constexpr (std::is_same_v<T, L3::Assign>)
          new_function.instructions.push_back(inst);
        else if constexpr (std::is_same_v<T, L3::CallL3>)
          new_function.instructions.push_back(inst);
        else if constexpr (std::is_same_v<T, L3::CallAssignL3>)
          new_function.instructions.push_back(inst);
        else if constexpr (std::is_same_v<T, L3::CallPrint>)
          new_function.instructions.push_back(inst);
        else if constexpr (std::is_same_v<T, L3::CallInput>)
          new_function.instructions.push_back(inst);
        else if constexpr (std::is_same_v<T, L3::CallTupleError>)
          new_function.instructions.push_back(inst);
        else if constexpr (std::is_same_v<T, L3::CallTensorError>)
          new_function.instructions.push_back(inst);
        else if constexpr (std::is_same_v<T, BinOp>) {
          new_function.instructions.push_back(BinOp_to_l3op(inst));
        } else if constexpr (std::is_same_v<T, TypeVar>) {
        }
        // TODO ArrayRead, ArrayWrite, Length, NewArray, NewTuple
      },
                 instr);
    }
    // pattern match the terminal instructions after, depending on whether it can fallthrough to the next bb
    bool has_next = (block_idx + 1 < f.blocks.size());
    std::visit([&](auto &terminal_inst) {
      using T = std::decay_t<decltype(terminal_inst)>;
      if constexpr (std::is_same_v<T, L3::Return>)
        new_function.instructions.push_back(terminal_inst);
      else if constexpr (std::is_same_v<T, L3::ReturnT>)
        new_function.instructions.push_back(terminal_inst); // push returns as they are
      else if constexpr (std::is_same_v<T, L3::Goto>) {
        if (!has_next || !(f.blocks[block_idx + 1].lbl == terminal_inst.target)) // if it doesnt't have a basic block after, or the next basic block does not match the target
          new_function.instructions.push_back(L3::brlabel{terminal_inst.target});
      } else if constexpr (std::is_same_v<T, BrCond>) {
        new_function.instructions.push_back(L3::brtlabel{terminal_inst.cond, terminal_inst.true_label}); // only push the first branch unless the next block matches the else label of the cond
        bool else_next = has_next && f.blocks[block_idx + 1].lbl == terminal_inst.false_label;
        if (!else_next)
          new_function.instructions.push_back(L3::brlabel{terminal_inst.false_label});
      }
    },
               block.terminal);
  }
  return new_function;
}
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
    std::vector<trace> traces;                             // define a list of traces for the function
    for (auto &orig_block : fn.blocks) {
      auto it = f_bb.find(orig_block);
      if (it == f_bb.end())
        continue;
      trace tr;
      bb first_bb = *it;
      f_bb.erase(it);
      trace_bb tbb = bb_to_trace_bb(first_bb); // initialize a trace basic block with mark set to false
      while (!tbb.mark) {
        tbb.mark = true;
        tr.blocks.push_back(tbb);
        std::vector<L3::label> succs = successors(tbb);
        tbb = select_next(f_bb, tbb, succs);
      }
      traces.push_back(tr);
    }
    linearized_p.functions.push_back(fIR{fn.f_type, fn.name, fn.parameters, traces_to_bbs(traces)});
  };
  return linearized_p;
};
L3::pl3 pIR_to_pL3(pIR &p) {
  std::vector<L3::fl3> function_list;
  for (auto &fn : p.functions) {
    function_list.push_back(fIR_to_fl3(fn));
  }
  return L3::pl3{
      function_list};
};

void generate_code(pIR &p) {
  pIR linearized_p;
  // linearize CFG
  linearized_p = linearize_CFG(p);
  L3::pl3 transformed_p;
  transformed_p = pIR_to_pL3(linearized_p);
  std::ofstream outputFile;
  outputFile.open("prog.L3");

  // build the L3 code for each of the funtions inside of the ast/program
  for (auto &fn : transformed_p.functions) {
    print_functionl3(fn, true, &outputFile);
  };

  outputFile.close();
}
} // namespace IR