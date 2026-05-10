#include "L3.h"
#include "parser.h"
#include "parsing/primitives.h"
#include "tiles.h"
#include <fstream>
#include <functional>
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

static int count_uses(const Tree &tree, const var &v) {
  return std::visit(overloaded{
                        [&](const Leaf &l) -> int {
                          return is_var(l.value, v) ? 1 : 0;
                        },
                        [&](const std::unique_ptr<Inner> &node) -> int {
                          return count_uses(node->left, v) + count_uses(node->right, v);
                        },
                        [&](const std::unique_ptr<CallNode> &node) -> int {
                          int count = 0;
                          for (const auto &arg : node->args)
                            if (is_var(arg, v))
                              count++;
                          return count;
                        }},
                    tree);
}
void collect_var(const Tree &tree, std::vector<var> &acc_vector) {
  std::visit(overloaded{
                 [&](const Leaf &l) {
                   if (auto *v = std::get_if<var>(&l.value))
                     acc_vector.push_back(*v);
                 },
                 [&](const std::unique_ptr<Inner> &node) {
                   collect_var(node->left, acc_vector);
                   collect_var(node->right, acc_vector);
                 },
                 [&](const std::unique_ptr<CallNode> &node) {
                   for (const auto &arg : node->args)
                     if (auto *v = std::get_if<var>(&arg))
                       acc_vector.push_back(*v);
                 }},
             tree);
}

std::vector<var> collect_used_vars(Tree &tree) {
  std::vector<var> collected_vars;
  collect_var(tree, collected_vars);
  return collected_vars;
};

// rest of context does not use the variable.
bool uses_zero(std::vector<Tree>::iterator begin, std::vector<Tree>::iterator end, const var &v) {
  for (auto it = begin; it != end; ++it)
    if (count_uses(*it, v) > 0)
      return false;
  return true;
}

// read as T1 uses %var once?
static Tree *find_single_use(Tree &tree, const var &v) {
  Tree *found = nullptr;
  int count = 0;

  std::function<void(Tree &)> traverse = [&](Tree &t) {
    std::visit(overloaded{
                   [&](Leaf &l) {
                     if (is_var(l.value, v)) {
                       count++;
                       found = &t;
                     }
                   },
                   [&](std::unique_ptr<Inner> &node) {
                     traverse(node->left);
                     traverse(node->right);
                   },
                   [&](std::unique_ptr<CallNode> &) {}},
               t);
  };

  traverse(tree);
  return count == 1 ? found : nullptr;
}
// read as doest tree have dest?
bool tree_dest(Tree &T2, var &variable_of_interest) {
  std::visit(overloaded{
                 [](const std::unique_ptr<Inner> &node) {
                   if (node->dest.has_value()) {
                     var variable_of_interest = node->dest.value();
                     return true;
                   }
                   return false;
                 },
                 [](const std::unique_ptr<CallNode> &node) {
                   if (node->dest.has_value()) {
                     var variable_of_interest = node->dest.value();
                     return true;
                   };
                   return false;
                 },
                 [](const Leaf &) {
                   return false;
                 }},
             T2);
  __builtin_unreachable();
}

bool is_memory_instruction(Tree &T2) {
  return std::visit(overloaded{
                        [](const std::unique_ptr<Inner> &node) {
                          return node->op == Op::Load || node->op == Op::Store;
                        },
                        [](const std::unique_ptr<CallNode> &node) {
                          return false;
                        },
                        [](const Leaf &) {
                          return false;
                        }},
                    T2);
};

std::vector<std::vector<Tree>> merge_trees(std::vector<std::vector<Tree>> &tree_contexts) {
  std::vector<std::vector<Tree>> merged(tree_contexts.size());
  for (int tree_context_idx = 0; tree_context_idx < tree_contexts.size(); tree_context_idx++) {
    std::vector<Tree> &tree_context = tree_contexts[tree_context_idx];
    std::vector<Tree> context_output;
    for (int t1_idx = 0; t1_idx < (int)tree_context.size(); t1_idx++) {
      Tree T1 = std::move(tree_context[t1_idx]);
      for (const var &v : collect_used_vars(T1)) {
        int t2_idx = -1;
        var def;
        // iterate through the trees in reverse order to find where did a tree define the variable it used.
        for (int j = (int)context_output.size() - 1; j >= 0; j--) {
          if (tree_dest(context_output[j], def) && def == v && !is_memory_instruction(context_output[j])) {
            t2_idx = j; // break loop if found
            break;
          }
        }
        if (t2_idx == -1)
          continue; // MERGE CONFLICT !

        Tree *loc = find_single_use(T1, v);
        if (!loc) // check if T1 indeed uses it just one time
          continue;

        // no uses of %v between T2 and T1 and no memory operations
        bool v_unused_between_t2_and_t1 = true;
        bool no_mem_ops_between_t2_and_t1 = true;
        for (int j = t2_idx + 1; j < (int)context_output.size(); j++) {
          if (count_uses(context_output[j], v) > 0) {
            v_unused_between_t2_and_t1 = false;
            break;
          }
          if (is_memory_instruction(context_output[j])) {
            no_mem_ops_between_t2_and_t1 = false;
            break;
          }
        }

        if (!v_unused_between_t2_and_t1 || !no_mem_ops_between_t2_and_t1)
          continue; // MERGE CONFLICT !

        // check for no uses of v after T1 inside of the context
        if (!uses_zero(tree_context.begin() + t1_idx + 1, tree_context.end(), v))
          continue; // MERGE CONFLICT !!

        // check that there are no redefinitions of T2's used variables in between the t1 and t2.
        bool no_redefs = true;
        for (const var &u : collect_used_vars(context_output[t2_idx]))
          for (int j = t2_idx + 1; j < (int)context_output.size(); j++) {
            var d;
            if (tree_dest(context_output[j], d) && d == u) {
              no_redefs = false;
              break;
            }
          }
        if (!no_redefs)
          continue; // MERGE CONFLICT!

        *loc = std::move(context_output[t2_idx]);              // perform the merge by putting t2 in the location where the variable is used.
        context_output.erase(context_output.begin() + t2_idx); // remove T2 from context output if merge is successful
        break;
      }
      context_output.push_back(std::move(T1));
    };
    merged[tree_context_idx].insert(
        merged[tree_context_idx].end(),
        std::make_move_iterator(context_output.begin()),
        std::make_move_iterator(context_output.end()));
  }
  return std::move(merged);
};

s tile_node(Tree &tree, std::vector<i> &out, int &lbl_counter, int &temp_counter,
            std::unordered_map<label, label> &label_map, int &call_counter) {
  TileCtx context_obj{out, lbl_counter, temp_counter, label_map, call_counter};
  return std::visit(overloaded{

                        [](Leaf &l) -> s {
                          return l.value;
                        },
                        [&](std::unique_ptr<Inner> &node) -> s {
                          switch (node->op) {
                          case Op::Assign: {
                            return atomic_assign_tile(node, context_obj);
                          }
                          case Op::Load: {
                            // auto r = match_large_tile(Op::Load, node, context_obj);
                            // if (r)
                            // return *r;
                            return atomic_load_tile(node, context_obj);
                          }
                          case Op::Store: {
                            // auto r = match_large_tile(Op::Store, node, context_obj);
                            // if (r)
                            // return *r;
                            return atomic_store_tile(node, context_obj);
                          }
                          case Op::Add:
                          case Op::Sub:
                          case Op::Mul:
                          case Op::And:
                          case Op::Shl:
                          case Op::Shr: {
                            // auto r = match_large_tile(node->op, node, context_obj);
                            // if (r)
                            // return *r;
                            return atomic_Aop_tile(node, context_obj);
                          }
                          case Op::Lt:
                          case Op::Le:
                          case Op::Eq: {
                            return atomic_CmpOp_tile(node, context_obj);
                          }
                          case Op::Gt: {
                            std::swap(node->left, node->right);
                            node->op = Op::Lt;
                            return atomic_CmpOp_tile(node, context_obj);
                          }
                          case Op::Ge: {
                            std::swap(node->left, node->right);
                            node->op = Op::Le;
                            return atomic_CmpOp_tile(node, context_obj);
                          }
                          case Op::Return: {
                            return atomic_return_tile(node, context_obj);
                          }
                          case Op::Label: {
                            return atomic_label_tile(node, context_obj);
                          }
                          case Op::Br: {
                            return atomic_br_tile(node, context_obj);
                          }
                          case Op::BrCond: {
                            // auto r = match_large_tile(Op::BrCond, node, context_obj);
                            // if (r)
                            // return *r;
                            return atomic_cond_tile(node, context_obj);
                          }
                          default:
                            __builtin_unreachable();
                          }
                        },

                        [&](std::unique_ptr<CallNode> &node) -> s {
                          return atomic_call_tile(node, context_obj);
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