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
  for (auto &block : f.blocks) {
    for (auto &instr : block.instructions) {
      std::visit([&](auto &inst) {
        using T = std::decay_t<decltype(inst)>;
        if constexpr (std::is_same_v<T, L3::ReturnT>) {
          new_function.instructions.push_back(Assign{r::rax, t_to_s(inst.return_thing)});
          new_function.instructions.push_back(Return{});
        } else if constexpr (std::is_same_v<T, L3::AopL3>) {
          bool right_is_dest = std::visit([&](auto &d) -> bool {
            using DT = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<DT, var>)
              return std::holds_alternative<var>(inst.right) && std::get<var>(inst.right).name == d.name;
            else if constexpr (std::is_same_v<DT, r>)
              return std::holds_alternative<r>(inst.right) && std::get<r>(inst.right) == d;
            return false;
          }, inst.dest);
          if (right_is_dest && inst.op == aop::sub) {
            // dest = left - right, but right == dest
            // generate a temporary variable... because the operation is non commutative
            var tmp{"__sub_tmp_" + std::to_string(lbl_counter) + "_" + std::to_string(temp_counter++)};
            new_function.instructions.push_back(Assign{w{tmp}, t_to_s(inst.left)});
            new_function.instructions.push_back(AopInst{w{tmp}, inst.op, inst.right});
            new_function.instructions.push_back(Assign{inst.dest, s{tmp}});
          } else if (right_is_dest) {
            new_function.instructions.push_back(Assign{inst.dest, w_to_s(inst.dest)});
            new_function.instructions.push_back(AopInst{inst.dest, inst.op, inst.left});
          } else {
            new_function.instructions.push_back(Assign{inst.dest, t_to_s(inst.left)});
            new_function.instructions.push_back(AopInst{inst.dest, inst.op, inst.right});
          }
        } else if constexpr (std::is_same_v<T, L3::LabelInst>) {  // for all instructions that have labels or have a label in their variant, rename labels according to function scope
          new_function.instructions.push_back(LabelInst{check_map_and_rename_label(label_map, inst.lbl, lbl_counter)});
        } else if constexpr (std::is_same_v<T, L3::brlabel>) {
          new_function.instructions.push_back(Goto{check_map_and_rename_label(label_map, inst.lbl, lbl_counter)});
        } else if constexpr (std::is_same_v<T, brtlabel>) {
          label renamed_lbl = check_map_and_rename_label(label_map, inst.lbl, lbl_counter);
          new_function.instructions.push_back(Cjump{inst.cond, cmp::eq, t{N{1}}, renamed_lbl});
        } else if constexpr (std::is_same_v<T, StoreL3>) { // if the src contains a label, then check_map and rename label, else thread the same src
          if (std::holds_alternative<label>(inst.src)) {
            label renamed = check_map_and_rename_label(label_map, std::get<label>(inst.src), lbl_counter);
            new_function.instructions.push_back(MemWrite{w{inst.dest}, 0, s{renamed}});
          } else {
            new_function.instructions.push_back(MemWrite{w{inst.dest}, 0, inst.src});
          }
        } else if constexpr (std::is_same_v<T, LoadL3>) {
          new_function.instructions.push_back(MemRead{w{inst.dest}, w{inst.src}, 0});
        } else if constexpr (std::is_same_v<T, Compare>) {
          if (inst.op == cmp::gt)
            new_function.instructions.push_back(Compare{inst.dest, inst.right, cmp::lt, inst.left});
          else if (inst.op == cmp::gte)
            new_function.instructions.push_back(Compare{inst.dest, inst.right, cmp::le, inst.left});
          else
            new_function.instructions.push_back(inst);
        } else if constexpr (std::is_same_v<T, Assign>) {
          // if the src contains a label, then check_map and rename label, else thread the same src
          if (std::holds_alternative<label>(inst.src)) {
            label renamed = check_map_and_rename_label(label_map, std::get<label>(inst.src), lbl_counter);
            new_function.instructions.push_back(Assign{inst.dest, s{renamed}});
          }
          else {
          new_function.instructions.push_back(inst);                                                                                                                         
      } 
        } else if constexpr (std::is_same_v<T, CallL3>) {
          auto arg_insts = regs_to_args(inst.args);
          new_function.instructions.insert(new_function.instructions.end(), arg_insts.begin(), arg_insts.end());
          bool is_user_call = std::holds_alternative<u>(inst.callee);
          if (is_user_call) {
            label ret_label{":ret_" + std::to_string(lbl_counter) + "_" + std::to_string(call_counter++)};
            new_function.instructions.push_back(MemWrite{w{r::rsp}, -8, s{ret_label}});
            emit_call(new_function, inst.callee, (int64_t)inst.args.size());
            new_function.instructions.push_back(LabelInst{ret_label});
          } else {
            call_counter++;
            emit_call(new_function, inst.callee, (int64_t)inst.args.size());
          }
        } else if constexpr (std::is_same_v<T, CallAssignL3>) {
          auto arg_insts = regs_to_args(inst.callexpr.args);
          new_function.instructions.insert(new_function.instructions.end(), arg_insts.begin(), arg_insts.end());
          bool is_user_call = std::holds_alternative<u>(inst.callexpr.callee);
          if (is_user_call) {
            label ret_label{":ret_" + std::to_string(lbl_counter) + "_" + std::to_string(call_counter++)};
            new_function.instructions.push_back(MemWrite{w{r::rsp}, -8, s{ret_label}});
            emit_call(new_function, inst.callexpr.callee, (int64_t)inst.callexpr.args.size());
            new_function.instructions.push_back(LabelInst{ret_label});
          } else {
            call_counter++;
            emit_call(new_function, inst.callexpr.callee, (int64_t)inst.callexpr.args.size());
          }
          new_function.instructions.push_back(Assign{w{inst.dest}, s{r::rax}});
        }else {
            new_function.instructions.push_back(inst);
          } },
                 instr);
    };
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