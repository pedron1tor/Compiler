#include "L3.h"
#include "parser.h"
#include "parsing/primitives.h"
#include <fstream>
#include <unordered_map>

namespace L3 {

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
void emit_call(std::vector<i> &new_function_instructions, callee_type &c, int64_t num_args) {
  // if the callee variant is of type u, we have to rewrap it with a standard L2 call...
  std::visit([&](auto &callee_val) {
    using CT = std::decay_t<decltype(callee_val)>;
    if constexpr (std::is_same_v<CT, u>)
      new_function_instructions.push_back(Call{callee_val, N{num_args}});
    else if constexpr (std::is_same_v<CT, CallTensorError>) {
      new_function_instructions.push_back(CallTensorError{num_args});
    } else
      new_function_instructions.push_back(callee_val);
  },
             c);
};

std::vector<context> identify_contexts(std::vector<i> &instructions) {
  std::vector<context> contexts;
  context context_obj;
  for (auto &instr : instructions) {
    std::visit([&](auto &inst) {
      using T = std::decay_t<decltype(inst)>;
      if constexpr (std::is_same_v<T, L3::Return> || std::is_same_v<T, L3::ReturnT> || std::is_same_v<T, L3::CallL3> || std::is_same_v<T, L3::LabelInst> || std::is_same_v<T, L3::brlabel> || std::is_same_v<T, L3::brtlabel>) {
        context_obj.instructions.push_back(inst);
        contexts.push_back(context_obj);
        context_obj.instructions.clear();
      } else {
        context_obj.instructions.push_back(inst);
      };
    },
               instr);
  };
  if (!context_obj.instructions.empty())
    contexts.push_back(std::move(context_obj));
  return contexts;
};

std::vector<std::vector<Tree>> transform_contexts_to_trees(std::vector<context> &contexts) {
  std::vector<std::vector<Tree>> tree_contexts;
  for (context &context_obj : contexts) {
    std::vector<Tree> tree_vector;
    // helper functions to reduce boilerplate
    auto makeInnerNode = [&](Op op, var dest = {},
                             std::optional<Tree> left = std::nullopt,
                             std::optional<Tree> right = std::nullopt) {
      auto node = std::make_unique<Inner>();
      node->op = op;
      node->dest = dest;
      if (left)
        node->left = std::move(*left);
      if (right)
        node->right = std::move(*right);
      tree_vector.push_back(Tree{std::move(node)});
    };
    auto makeCallNode = [&](std::optional<var> dest, callee_type callee, std::vector<s> args) {
      auto node = std::make_unique<CallNode>();
      node->dest = dest;
      node->callee = callee;
      node->args = std::move(args);
      tree_vector.push_back(Tree{std::move(node)});
    };

    for (auto &instr : context_obj.instructions) {
      std::visit([&](auto &inst) {
        using T = std::decay_t<decltype(inst)>;

        if constexpr (std::is_same_v<T, L3::Return>)
          makeInnerNode(Op::Return);

        else if constexpr (std::is_same_v<T, L3::ReturnT>)
          makeInnerNode(Op::Return, {}, Tree{Leaf{t_to_s(inst.return_thing)}});

        else if constexpr (std::is_same_v<T, L3::AopL3>)
          makeInnerNode(toOp(inst.op), std::get<var>(inst.dest), Tree{Leaf{t_to_s(inst.left)}}, Tree{Leaf{t_to_s(inst.right)}});

        else if constexpr (std::is_same_v<T, L3::Compare>)
          makeInnerNode(toCmpOp(inst.op), std::get<var>(inst.dest), Tree{Leaf{t_to_s(inst.left)}}, Tree{Leaf{t_to_s(inst.right)}});

        else if constexpr (std::is_same_v<T, L3::Assign>)
          makeInnerNode(Op::Assign, std::get<var>(inst.dest), Tree{Leaf{inst.src}});

        else if constexpr (std::is_same_v<T, L3::LoadL3>)
          makeInnerNode(Op::Load, inst.dest, Tree{Leaf{inst.src}});

        else if constexpr (std::is_same_v<T, L3::StoreL3>)
          makeInnerNode(Op::Store, {}, Tree{Leaf{inst.dest}}, Tree{Leaf{inst.src}});

        else if constexpr (std::is_same_v<T, L3::LabelInst>)
          makeInnerNode(Op::Label, {}, Tree{Leaf{s{inst.lbl}}});

        else if constexpr (std::is_same_v<T, L3::brlabel>)
          makeInnerNode(Op::Br, {}, Tree{Leaf{s{inst.lbl}}});

        else if constexpr (std::is_same_v<T, L3::brtlabel>)
          makeInnerNode(Op::BrCond, {}, Tree{Leaf{t_to_s(inst.cond)}}, Tree{Leaf{s{inst.lbl}}});

        else if constexpr (std::is_same_v<T, L3::CallL3>) {
          std::vector<s> args_vars;
          for (auto &a : inst.args)
            args_vars.push_back(t_to_s(a));
          makeCallNode(std::nullopt, inst.callee, std::move(args_vars));
        }

        else if constexpr (std::is_same_v<T, L3::CallAssignL3>) {
          std::vector<s> args_vars;
          for (auto &a : inst.callexpr.args)
            args_vars.push_back(t_to_s(a));
          makeCallNode(inst.dest, inst.callexpr.callee, std::move(args_vars));
        }
      },
                 instr);
    }
    tree_contexts.push_back(std::move(tree_vector));
  }
  return tree_contexts;
};

std::vector<std::vector<Tree>> merge_trees(std::vector<std::vector<Tree>> &tree_contexts) {
  return std::move(tree_contexts);
};

s tile_node(Tree &tree, std::vector<i> &out, int &lbl_counter, int &temp_counter,
            std::unordered_map<label, label> &label_map, int &call_counter) {

  return std::visit(overloaded{

                        [](Leaf &l) -> s {
                          return l.value;
                        },
                        [&](std::unique_ptr<Inner> &node) -> s {
                          switch (node->op) {
                          case Op::Assign: {
                            s src = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            if (std::holds_alternative<label>(src))
                              src = s{check_map_and_rename_label(label_map, std::get<label>(src), lbl_counter)};
                            out.push_back(Assign{w{node->dest}, src});
                            return s{node->dest};
                          }
                          case Op::Load: {
                            s base = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            out.push_back(MemRead{w{node->dest}, s_to_w(base), 0});
                            return s{node->dest};
                          }
                          case Op::Store: {
                            s addr = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            s val = tile_node(node->right, out, lbl_counter, temp_counter, label_map, call_counter);
                            if (std::holds_alternative<label>(val)) {
                              label renamed = check_map_and_rename_label(label_map, std::get<label>(val), lbl_counter);
                              out.push_back(MemWrite{s_to_w(addr), 0, s{renamed}});
                            } else {
                              out.push_back(MemWrite{s_to_w(addr), 0, val});
                            }
                            return s{var{}};
                          }
                          case Op::Add:
                          case Op::Sub:
                          case Op::Mul:
                          case Op::And:
                          case Op::Shl:
                          case Op::Shr: {
                            aop aop_val = op_to_aop(node->op);
                            s lv = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            s rv = tile_node(node->right, out, lbl_counter, temp_counter, label_map, call_counter);
                            bool right_is_dest = std::holds_alternative<var>(rv) &&
                                                 std::get<var>(rv).name == node->dest.name;
                            if (right_is_dest && aop_val == aop::sub) {
                              var tmp{"__sub_tmp_" + std::to_string(lbl_counter) + "_" + std::to_string(temp_counter++)};
                              out.push_back(Assign{w{tmp}, lv});
                              out.push_back(AopInst{w{tmp}, aop_val, s_to_t(rv)});
                              out.push_back(Assign{w{node->dest}, s{tmp}});
                            } else if (right_is_dest) {
                              out.push_back(Assign{w{node->dest}, rv});
                              out.push_back(AopInst{w{node->dest}, aop_val, s_to_t(lv)});
                            } else {
                              out.push_back(Assign{w{node->dest}, lv});
                              out.push_back(AopInst{w{node->dest}, aop_val, s_to_t(rv)});
                            }
                            return s{node->dest};
                          }
                          case Op::Lt:
                          case Op::Le:
                          case Op::Eq: {
                            s lv = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            s rv = tile_node(node->right, out, lbl_counter, temp_counter, label_map, call_counter);
                            cmp op_val = (node->op == Op::Lt) ? cmp::lt : (node->op == Op::Le) ? cmp::le
                                                                                               : cmp::eq;
                            out.push_back(Compare{w{node->dest}, s_to_t(lv), op_val, s_to_t(rv)});
                            return s{node->dest};
                          }
                          case Op::Gt: {
                            s lv = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            s rv = tile_node(node->right, out, lbl_counter, temp_counter, label_map, call_counter);
                            out.push_back(Compare{w{node->dest}, s_to_t(rv), cmp::lt, s_to_t(lv)});
                            return s{node->dest};
                          }
                          case Op::Ge: {
                            s lv = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            s rv = tile_node(node->right, out, lbl_counter, temp_counter, label_map, call_counter);
                            out.push_back(Compare{w{node->dest}, s_to_t(rv), cmp::le, s_to_t(lv)});
                            return s{node->dest};
                          }
                          case Op::Return: {
                            bool has_val = !std::holds_alternative<Leaf>(node->left) ||
                                           !(std::holds_alternative<var>(std::get<Leaf>(node->left).value) &&
                                             std::get<var>(std::get<Leaf>(node->left).value).name.empty());
                            // handle the case where it's returnt
                            if (has_val) {
                              s val = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                              out.push_back(Assign{w{r::rax}, val});
                            }
                            out.push_back(Return{});
                            return s{var{}};
                          }
                          case Op::Label: {
                            label lbl = std::get<label>(std::get<Leaf>(node->left).value);
                            out.push_back(LabelInst{check_map_and_rename_label(label_map, lbl, lbl_counter)});
                            return s{var{}};
                          }
                          case Op::Br: {
                            label lbl = std::get<label>(std::get<Leaf>(node->left).value);
                            out.push_back(Goto{check_map_and_rename_label(label_map, lbl, lbl_counter)});
                            return s{var{}};
                          }
                          case Op::BrCond: {
                            s cond = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
                            label lbl = std::get<label>(std::get<Leaf>(node->right).value);
                            label renamed = check_map_and_rename_label(label_map, lbl, lbl_counter);
                            out.push_back(Cjump{s_to_t(cond), cmp::eq, t{N{1}}, renamed});
                            return s{var{}};
                          }
                          default:
                            __builtin_unreachable();
                          }
                        },

                        [&](std::unique_ptr<CallNode> &node) -> s {
                          std::vector<t> arg_ts;
                          for (auto &a : node->args)
                            arg_ts.push_back(s_to_t(a));
                          auto arg_insts = regs_to_args(arg_ts);
                          out.insert(out.end(), arg_insts.begin(), arg_insts.end());
                          bool is_user_call = std::holds_alternative<u>(node->callee);
                          if (is_user_call) {
                            label ret_label{":ret_" + std::to_string(lbl_counter) + "_" + std::to_string(call_counter++)};
                            out.push_back(MemWrite{w{r::rsp}, -8, s{ret_label}});
                            emit_call(out, node->callee, (int64_t)node->args.size());
                            out.push_back(LabelInst{ret_label});
                          } else {
                            call_counter++;
                            emit_call(out, node->callee, (int64_t)node->args.size());
                          }
                          if (node->dest.has_value()) {
                            out.push_back(Assign{w{*node->dest}, s{r::rax}});
                            return s{*node->dest};
                          }
                          return s{var{}};
                        }},
                    tree);
}

std::vector<i> tiling(std::vector<std::vector<Tree>> &tree_contexts, std::unordered_map<label, label> &label_map, int &lbl_counter) {
  int call_counter = 0;
  int temp_counter = 0;
  std::vector<i> out;
  for (auto &tree_context : tree_contexts)
    for (auto &tree : tree_context)
      tile_node(tree, out, lbl_counter, temp_counter, label_map, call_counter);
  return out;
};

fl2 fl3_to_fl2(fl3 &f, std::unordered_map<label, label> &label_map, int &lbl_counter) {
  fl2 new_function;
  new_function.name = f.name;
  new_function.num_args = f.vars.size();
  // assign a register for each of the parameters
  std::vector<i> register_assigns = args_to_regs(f.vars);
  // append to the instructions of the function.
  new_function.instructions.insert(new_function.instructions.end(), register_assigns.begin(), register_assigns.end());
  // identify contexts
  std::vector<context> contexts = identify_contexts(f.instructions);
  // tree generation
  std::vector<std::vector<Tree>> tree_contexts = transform_contexts_to_trees(contexts);
  // tree merging
  std::vector<std::vector<Tree>> tree_contexts_merged = merge_trees(tree_contexts);
  // tiling...
  std::vector<i> tiled = tiling(tree_contexts_merged, label_map, lbl_counter);
  new_function.instructions.insert(new_function.instructions.end(), tiled.begin(), tiled.end());
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