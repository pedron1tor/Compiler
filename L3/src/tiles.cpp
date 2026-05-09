#include "tiles.h"
#include "parsing/primitives.h"

namespace L3 {
/*
Atomic tiles!! For only one L3 instruction
*/
s atomic_assign_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                     int &lbl_counter, int &temp_counter,
                     std::unordered_map<label, label> &label_map, int &call_counter) {
  s src = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
  if (std::holds_alternative<label>(src))
    src = s{check_map_and_rename_label(label_map, std::get<label>(src), lbl_counter)};
  out.push_back(Assign{w{node->dest}, src});
  return s{node->dest};
}
s atomic_load_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                   int &lbl_counter, int &temp_counter,
                   std::unordered_map<label, label> &label_map, int &call_counter) {
  s base = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
  out.push_back(MemRead{w{node->dest}, s_to_w(base), 0});
  return s{node->dest};
}
s atomic_store_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                    int &lbl_counter, int &temp_counter,
                    std::unordered_map<label, label> &label_map, int &call_counter) {
  s addr = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
  s val = tile_node(node->right, out, lbl_counter, temp_counter, label_map, call_counter);
  if (std::holds_alternative<label>(val)) {
    label renamed = check_map_and_rename_label(label_map, std::get<label>(val), lbl_counter);
    out.push_back(MemWrite{s_to_w(addr), 0, s{renamed}});
  } else {
    out.push_back(MemWrite{s_to_w(addr), 0, val});
  }
  return s{var{}};
};

s atomic_Aop_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                  int &lbl_counter, int &temp_counter,
                  std::unordered_map<label, label> &label_map, int &call_counter) {
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
};

s atomic_CmpOp_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                    int &lbl_counter, int &temp_counter,
                    std::unordered_map<label, label> &label_map, int &call_counter) {
  s lv = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
  s rv = tile_node(node->right, out, lbl_counter, temp_counter, label_map, call_counter);
  cmp op_val = (node->op == Op::Lt) ? cmp::lt : (node->op == Op::Le) ? cmp::le
                                                                     : cmp::eq;
  out.push_back(Compare{w{node->dest}, s_to_t(lv), op_val, s_to_t(rv)});
  return s{node->dest};
}
s atomic_return_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                     int &lbl_counter, int &temp_counter,
                     std::unordered_map<label, label> &label_map, int &call_counter) {
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
};
s atomic_label_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                    int &lbl_counter, int &temp_counter,
                    std::unordered_map<label, label> &label_map, int &call_counter) {
  label lbl = std::get<label>(std::get<Leaf>(node->left).value);
  out.push_back(LabelInst{check_map_and_rename_label(label_map, lbl, lbl_counter)});
  return s{var{}};
};
s atomic_br_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                 int &lbl_counter, int &temp_counter,
                 std::unordered_map<label, label> &label_map, int &call_counter) {
  label lbl = std::get<label>(std::get<Leaf>(node->left).value);
  out.push_back(Goto{check_map_and_rename_label(label_map, lbl, lbl_counter)});
  return s{var{}};
};
s atomic_cond_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                   int &lbl_counter, int &temp_counter,
                   std::unordered_map<label, label> &label_map, int &call_counter) {
  s cond = tile_node(node->left, out, lbl_counter, temp_counter, label_map, call_counter);
  label lbl = std::get<label>(std::get<Leaf>(node->right).value);
  label renamed = check_map_and_rename_label(label_map, lbl, lbl_counter);
  out.push_back(Cjump{s_to_t(cond), cmp::eq, t{N{1}}, renamed});
  return s{var{}};
};
s atomic_call_tile(std::unique_ptr<CallNode> &node, std::vector<i> &out,
                   int &lbl_counter, int &temp_counter,
                   std::unordered_map<label, label> &label_map, int &call_counter) {
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
};
} // namespace L3
