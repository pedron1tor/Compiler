#include "L3.h"
#include "parser.h"
#include <fstream>
#include <unordered_map>
namespace L3 {
s t_to_s(t &t_item) {
  return std::visit([](auto &v) -> s { return s{v}; }, t_item);
}
s w_to_s(w &w_item) {
  return std::visit([](auto &v) -> s { return s{v}; }, w_item);
}
std::vector<i> // this function when we first go into the scope of an l2 function.
args_to_regs(std::vector<var> &variables) {
  // allocate the variables to the registers first, then if still remaining allocate stack space.
  std::vector<i> allocating_instructions;
  for (int reg_idx = 0; reg_idx < (int)std::min(6UL, variables.size()); reg_idx++) {
    allocating_instructions.push_back(Assign{w{variables[reg_idx]}, w_to_s(arg_registers[reg_idx])});
  }
  // continue allocating on the stack if more than 6 args
  int stack_arg_counter = 0;
  if (variables.size() > 6) {
    for (int reg_idx = 6; reg_idx < variables.size(); reg_idx++) {
      // allocate stack space for the remaning variables
      allocating_instructions.push_back(StackArg{w{
                                                     variables[reg_idx]},
                                                 N{stack_arg_counter}});
      stack_arg_counter += 8;
    };
  };

  return allocating_instructions;
};

std::vector<i>
regs_to_args(std::vector<t> &args) {
  std::vector<i> instructions;
  // allocate the variables to the registers first, then if still remaining allocate stack space.
  for (int idx = 0; idx < (int)std::min(6UL, args.size()); idx++) {
    instructions.push_back(Assign{w{arg_registers[idx]}, t_to_s(args[idx])});
  }
  // handle stack spilling for >6 args
  if (args.size() > 6) {
    for (int idx = 6; idx < (int)args.size(); idx++) {
      // since the return label will be at -8, we have to make sure that the arguments get put at -16, and onwards...
      instructions.push_back(MemWrite{w{r::rsp}, -(int64_t)((args.size() - idx + 1) * 8), t_to_s(args[idx])});
    }
  }
  return instructions;
}

label check_map_and_rename_label(std::unordered_map<label, label> &map, label &lbl, int &global_counter) {
  auto it = map.find(lbl);
  label new_label = label{lbl.name + "_global_" + std::to_string(global_counter)};
  if (it != map.end()) {
    std::string s = it->second.name;
    int number = std::stoi(s.substr(s.rfind('_') + 1));
    if (number == global_counter) {
      return label{s};
    } else {
      map[lbl] = new_label;
      return new_label;
    };
  } else {
    map[new_label] = new_label;
    return new_label;
  };
};
void emit_call(fl2 &new_function, callee_type &c, int64_t num_args) {
  // if the callee variant is of type u, we have to rewrap it with a standard L2 call...
  std::visit([&](auto &callee_val) {
    using CT = std::decay_t<decltype(callee_val)>;
    if constexpr (std::is_same_v<CT, u>)
      new_function.instructions.push_back(Call{callee_val, N{num_args}});
    else
      new_function.instructions.push_back(callee_val);
  },
             c);
};
fl2 fl3_to_fl2(fl3 &f, std::unordered_map<label, label> &label_map, int &lbl_counter) {
  fl2 new_function;
  new_function.name = f.name;
  new_function.num_args = f.vars.size();
  // assign a register for each of the parameters
  std::vector<i> register_assigns = args_to_regs(f.vars);
  // append to the instructions of the function.
  new_function.instructions.insert(new_function.instructions.end(), register_assigns.begin(), register_assigns.end());

  int call_counter = 0;
  int temp_counter = 0;
  for (auto &instr : f.instructions) {
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
            var tmp{"%__sub_tmp_" + std::to_string(lbl_counter) + "_" + std::to_string(temp_counter++)};
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
  }
  return new_function;
}

pl2 pl3_to_pl2(pl3 &p) {
  std::string entry_point = "@main";
  std::vector<fl2> function_list;
  int glob_lbl_counter = 0;
  // hash map, l3-labels -> l2-labels
  std::unordered_map<label, label> label_map;
  for (auto &fn : p.functions) {
    function_list.push_back(fl3_to_fl2(fn, label_map, glob_lbl_counter));
    glob_lbl_counter++;
  }
  return pl2{
      entry_point, function_list};
};
void generate_code(pl3 &p) {
  pl2 transformed_p;
  transformed_p = pl3_to_pl2(p);
  std::ofstream outputFile;
  outputFile.open("prog.L2");

  outputFile << "(" << transformed_p.entry_point.name << "\n";
  // build the L2 code for each of the funtions inside of the ast/program
  for (auto &fn : transformed_p.functions) {
    print_functionl2(fn, true, &outputFile);
  };
  outputFile << ")\n";

  outputFile.close();
}
} // namespace L3