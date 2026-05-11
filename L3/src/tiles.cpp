#include "tiles.h"
#include "parsing/primitives.h"

namespace L3 {
/*
Atomic tiles!! For only one L3 instruction
*/
// helper functiont to rename labels
s rename_if_label(s val, TileCtx &ctx) {
  if (auto *lbl = std::get_if<label>(&val))
    return s{check_map_and_rename_label(ctx.label_map, *lbl, ctx.lbl_counter)};
  return val;
}
s atomic_assign_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s src = rename_if_label(tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter), ctx);
  ctx.out.push_back(Assign{w{node->dest.value()}, src});
  return s{node->dest.value()};
}
s atomic_load_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s base = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  ctx.out.push_back(MemRead{w{node->dest.value()}, s_to_w(base), 0});
  return s{node->dest.value()};
}
s atomic_store_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s addr = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  s val = rename_if_label(tile_node(node->right, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter), ctx);
  ctx.out.push_back(MemWrite{s_to_w(addr), 0, val});
  return s{var{}};
};

s atomic_Aop_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  aop aop_val = op_to_aop(node->op);
  s lv = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  s rv = tile_node(node->right, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  bool right_is_dest = std::holds_alternative<var>(rv) &&
                       std::get<var>(rv).name == node->dest.value().name;
  if (right_is_dest && aop_val == aop::sub) {
    var tmp{"__sub_tmp_" + std::to_string(ctx.lbl_counter) + "_" + std::to_string(ctx.temp_counter++)};
    ctx.out.push_back(Assign{w{tmp}, lv});
    ctx.out.push_back(AopInst{w{tmp}, aop_val, s_to_t(rv)});
    ctx.out.push_back(Assign{w{node->dest.value()}, s{tmp}});
  } else if (right_is_dest) {
    ctx.out.push_back(Assign{w{node->dest.value()}, rv});
    ctx.out.push_back(AopInst{w{node->dest.value()}, aop_val, s_to_t(lv)});
  } else {
    ctx.out.push_back(Assign{w{node->dest.value()}, lv});
    ctx.out.push_back(AopInst{w{node->dest.value()}, aop_val, s_to_t(rv)});
  }
  return s{node->dest.value()};
};

s atomic_CmpOp_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s lv = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  s rv = tile_node(node->right, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  cmp op_val = (node->op == Op::Lt) ? cmp::lt : (node->op == Op::Le) ? cmp::le
                                                                     : cmp::eq;
  ctx.out.push_back(Compare{w{node->dest.value()}, s_to_t(lv), op_val, s_to_t(rv)});
  return s{node->dest.value()};
}
s atomic_return_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  bool has_val = !std::holds_alternative<Leaf>(node->left) ||
                 !(std::holds_alternative<var>(std::get<Leaf>(node->left).value) &&
                   std::get<var>(std::get<Leaf>(node->left).value).name.empty());
  // handle the case where it's returnt
  if (has_val) {
    s val = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
    ctx.out.push_back(Assign{w{r::rax}, val});
  }
  ctx.out.push_back(Return{});
  return s{var{}};
};
s atomic_label_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  label lbl = std::get<label>(std::get<Leaf>(node->left).value);
  ctx.out.push_back(LabelInst{check_map_and_rename_label(ctx.label_map, lbl, ctx.lbl_counter)});
  return s{var{}};
};
s atomic_br_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  label lbl = std::get<label>(std::get<Leaf>(node->left).value);
  ctx.out.push_back(Goto{check_map_and_rename_label(ctx.label_map, lbl, ctx.lbl_counter)});
  return s{var{}};
};
s atomic_cond_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s cond = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  label lbl = std::get<label>(std::get<Leaf>(node->right).value);
  label renamed = check_map_and_rename_label(ctx.label_map, lbl, ctx.lbl_counter);
  ctx.out.push_back(Cjump{s_to_t(cond), cmp::eq, t{N{1}}, renamed});
  return s{var{}};
};
s atomic_call_tile(std::unique_ptr<CallNode> &node, TileCtx &ctx) {
  std::vector<t> arg_ts;
  for (auto &a : node->args)
    arg_ts.push_back(s_to_t(a));
  auto arg_insts = regs_to_args(arg_ts);
  ctx.out.insert(ctx.out.end(), arg_insts.begin(), arg_insts.end());
  bool is_user_call = std::holds_alternative<u>(node->callee);
  if (is_user_call) {
    label ret_label{":ret_" + std::to_string(ctx.lbl_counter) + "_" + std::to_string(ctx.call_counter++)};
    ctx.out.push_back(MemWrite{w{r::rsp}, -8, s{ret_label}});
    emit_call(ctx.out, node->callee, (int64_t)node->args.size());
    ctx.out.push_back(LabelInst{ret_label});
  } else {
    ctx.call_counter++;
    emit_call(ctx.out, node->callee, (int64_t)node->args.size());
  }
  if (node->dest.has_value()) {
    ctx.out.push_back(Assign{w{*node->dest}, s{r::rax}});
    return s{*node->dest};
  }
  return s{var{}};
};

static bool is_leaf_var(const Tree &tree, const var &v) {
  return std::holds_alternative<Leaf>(tree) &&
         std::holds_alternative<var>(std::get<Leaf>(tree).value) &&
         std::get<var>(std::get<Leaf>(tree).value) == v;
}
static bool is_leaf_N(const Tree &tree) {
  return std::holds_alternative<Leaf>(tree) &&
         std::holds_alternative<N>(std::get<Leaf>(tree).value);
}
static int64_t get_leaf_N_value(const Tree &tree) {
  return std::get<N>(std::get<Leaf>(tree).value).value;
}
static int64_t log2_pow2(int64_t v) {
  if (v <= 0 || (v & (v - 1)) != 0)
    return -1;
  int64_t k = 0;
  while ((1LL << k) < v)
    k++;
  return k;
}
static Inner *as_inner(Tree &t) {
  auto *p = std::get_if<std::unique_ptr<Inner>>(&t);
  return p ? p->get() : nullptr;
}
static const Inner *as_inner(const Tree &t) {
  const auto *p = std::get_if<std::unique_ptr<Inner>>(&t);
  return p ? p->get() : nullptr;
}
static std::optional<int64_t> const_offset(Inner *node) {
  if (is_leaf_N(node->right)) {
    int64_t C = get_leaf_N_value(node->right);
    if (C % 8 == 0)
      return C;
  }
  if (is_leaf_N(node->left)) {
    int64_t C = get_leaf_N_value(node->left);
    if (C % 8 == 0)
      return C;
  }
  return std::nullopt;
}

/*
Large tiles!! For more than one L3 instruction
*/
s self_op_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s rv = tile_node(node->right, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  ctx.out.push_back(AopInst{w{node->dest.value()}, op_to_aop(node->op), s_to_t(rv)});
  return s{node->dest.value()};
}

s self_op_commutative_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s lv = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  ctx.out.push_back(AopInst{w{node->dest.value()}, op_to_aop(node->op), s_to_t(lv)});
  return s{node->dest.value()};
}

// dest <- dest + 1  -~> dest++
s increment_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  ctx.out.push_back(Increment{w{node->dest.value()}});
  return s{node->dest.value()};
}

// dest <- dest + 1  -~> dest--
s decrement_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  ctx.out.push_back(Decrement{w{node->dest.value()}});
  return s{node->dest.value()};
}

// multiplication by a power of 2... just do a lshift operation
// dest <- dest * 2^k
s mul_by_pow2_self_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  int64_t k = log2_pow2(get_leaf_N_value(node->right));
  ctx.out.push_back(SopNum{w{node->dest.value()}, sop::lshift, N{k}});
  return s{node->dest.value()};
}
// case when the assigned variable and the operand variable are not the same
s mul_by_pow2_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  s lv = tile_node(node->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  int64_t k = log2_pow2(get_leaf_N_value(node->right));
  ctx.out.push_back(Assign{w{node->dest.value()}, lv});
  ctx.out.push_back(SopNum{w{node->dest.value()}, sop::lshift, N{k}});
  return s{node->dest.value()};
}

// var <- base + (idx * E)   -~>  var @ base idx E
s at_addr_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  bool right_is_mul = as_inner(node->right) != nullptr;
  auto is_mul_with_scale = [](Inner *p) -> bool {
    if (!p || p->op != Op::Mul)
      return false;
    auto ok = [](const Tree &t) {
      if (!is_leaf_N(t))
        return false;
      int64_t v = get_leaf_N_value(t);
      return v == 1 || v == 2 || v == 4 || v == 8;
    };
    return ok(p->right) || ok(p->left);
  };
  bool right_is_mul = is_mul_with_scale(as_inner(node->right));
  Tree &mul_tree = right_is_mul ? node->right : node->left;
  Tree &base_tree = right_is_mul ? node->left : node->right;
  Inner *mul_inner = as_inner(mul_tree);
  bool n_is_right = is_leaf_N(mul_inner->right);
  Tree &idx_tree = n_is_right ? mul_inner->left : mul_inner->right;
  Tree &n_tree = n_is_right ? mul_inner->right : mul_inner->left;
  int64_t scale = get_leaf_N_value(n_tree);
  s base_val = tile_node(base_tree, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  s idx_val = tile_node(idx_tree, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  // keep scaled-index intermediate variable for potential future-context use
  if (mul_inner->dest.has_value()) {
    ctx.out.push_back(Assign{w{mul_inner->dest.value()}, idx_val});
    ctx.out.push_back(AopInst{w{mul_inner->dest.value()}, aop::mul, s_to_t(s{N{scale}})});
  }
  ctx.out.push_back(LoadAddr{w{node->dest.value()}, s_to_w(base_val), s_to_w(idx_val), scale});
  return s{node->dest.value()};
}

// var <- t1 cmp t2; br var :lbl   -~>  cjump t1 cmp t2 :lbl
s cjump_fusion_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  Inner *cmp_inner = as_inner(node->left);
  Op cmp_op = cmp_inner->op;
  bool flip = (cmp_op == Op::Gt || cmp_op == Op::Ge);
  s lv = tile_node(flip ? cmp_inner->right : cmp_inner->left, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  s rv = tile_node(flip ? cmp_inner->left : cmp_inner->right, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  cmp op_val = op_to_cmp(cmp_op);
  label lbl = std::get<label>(std::get<Leaf>(node->right).value);
  label renamed = check_map_and_rename_label(ctx.label_map, lbl, ctx.lbl_counter);
  ctx.out.push_back(Cjump{s_to_t(lv), op_val, s_to_t(rv), renamed});
  return s{var{}};
}

// tmp <- var + C; dst <- load tmp   -~>  dst <- mem var C
s load_with_offset_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  Inner *add_inner = as_inner(node->left);
  bool n_is_right = is_leaf_N(add_inner->right);
  Tree &base_tree = n_is_right ? add_inner->left : add_inner->right;
  int64_t C = get_leaf_N_value(n_is_right ? add_inner->right : add_inner->left);
  s base_val = tile_node(base_tree, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  if (add_inner->dest.has_value()) {
    ctx.out.push_back(Assign{w{add_inner->dest.value()}, base_val});
    ctx.out.push_back(AopInst{w{add_inner->dest.value()}, aop::add, s_to_t(s{N{C}})});
  }
  ctx.out.push_back(MemRead{w{node->dest.value()}, s_to_w(base_val), C});
  return s{node->dest.value()};
}

// tmp <- var + C; store tmp <- s   -~>  mem var C <- s
s store_with_offset_tile(std::unique_ptr<Inner> &node, TileCtx &ctx) {
  Inner *add_inner = as_inner(node->left);
  bool n_is_right = is_leaf_N(add_inner->right);
  Tree &base_tree = n_is_right ? add_inner->left : add_inner->right;
  int64_t C = get_leaf_N_value(n_is_right ? add_inner->right : add_inner->left);
  s base_val = tile_node(base_tree, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter);
  if (add_inner->dest.has_value()) {
    ctx.out.push_back(Assign{w{add_inner->dest.value()}, base_val});
    ctx.out.push_back(AopInst{w{add_inner->dest.value()}, aop::add, s_to_t(s{N{C}})});
  }
  s val = rename_if_label(tile_node(node->right, ctx.out, ctx.lbl_counter, ctx.temp_counter, ctx.label_map, ctx.call_counter), ctx);
  ctx.out.push_back(MemWrite{s_to_w(base_val), C, val});
  return s{var{}};
}

std::optional<s> match_large_tile(Op op, std::unique_ptr<Inner> &node, TileCtx &ctx) {
  switch (op) {
  case Op::Add:
  case Op::Sub:
  case Op::Mul:
  case Op::And:
  case Op::Shl:
  case Op::Shr: {
    bool left_is_dest = is_leaf_var(node->left, node->dest.value());
    bool right_is_dest = is_leaf_var(node->right, node->dest.value());

    if (op == Op::Add && left_is_dest && is_leaf_N(node->right) && get_leaf_N_value(node->right) == 1)
      return increment_tile(node, ctx);

    if (op == Op::Sub && left_is_dest && is_leaf_N(node->right) && get_leaf_N_value(node->right) == 1)
      return decrement_tile(node, ctx);

    if (op == Op::Mul && left_is_dest && is_leaf_N(node->right) && log2_pow2(get_leaf_N_value(node->right)) >= 0)
      return mul_by_pow2_self_tile(node, ctx);

    if (op == Op::Mul && is_leaf_N(node->right) && log2_pow2(get_leaf_N_value(node->right)) >= 0)
      return mul_by_pow2_tile(node, ctx);

    if (op == Op::Add) {
      auto is_valid_mul_child = [](const Tree &t) -> bool {
        const Inner *inner = as_inner(t);
        if (!inner || inner->op != Op::Mul)
          return false;
        auto has_valid_scale = [](const Tree &nt) -> bool {
          if (!is_leaf_N(nt))
            return false;
          int64_t v = get_leaf_N_value(nt);
          return v == 1 || v == 2 || v == 4 || v == 8;
        };
        return has_valid_scale(inner->right) || has_valid_scale(inner->left);
      };
      if (is_valid_mul_child(node->right) || is_valid_mul_child(node->left))
        return at_addr_tile(node, ctx);
    }

    if (left_is_dest)
      return self_op_tile(node, ctx);

    if (right_is_dest && (op == Op::Add || op == Op::Mul || op == Op::And))
      return self_op_commutative_tile(node, ctx);

    return std::nullopt;
  }
  case Op::Load: {
    if (Inner *ai = as_inner(node->left); ai && ai->op == Op::Add && const_offset(ai)) {
      bool n_is_right = is_leaf_N(ai->right);
      Tree &base_tree = n_is_right ? ai->left : ai->right;
      if (ai->dest.has_value() && is_leaf_var(base_tree, ai->dest.value()))
        return std::nullopt;
      return load_with_offset_tile(node, ctx);
    }
    return std::nullopt;
  }
  case Op::Store: {
    if (Inner *ai = as_inner(node->left); ai && ai->op == Op::Add && const_offset(ai)) {
      bool n_is_right = is_leaf_N(ai->right);
      Tree &base_tree = n_is_right ? ai->left : ai->right;
      if (ai->dest.has_value() && is_leaf_var(base_tree, ai->dest.value()))
        return std::nullopt;
      return store_with_offset_tile(node, ctx);
    }
    return std::nullopt;
  }
  case Op::BrCond: {
    if (Inner *li = as_inner(node->left); li) {
      Op cmp_op = li->op;
      if (cmp_op == Op::Lt || cmp_op == Op::Le || cmp_op == Op::Eq ||
          cmp_op == Op::Gt || cmp_op == Op::Ge)
        return cjump_fusion_tile(node, ctx);
    }
    return std::nullopt;
  }
  default:
    return std::nullopt;
  }
}
} // namespace L3
