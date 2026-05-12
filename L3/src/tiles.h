#pragma once
#include "L3.h"
#include <unordered_map>
#include <vector>

namespace L3 {

s tile_node(Tree &tree, std::vector<i> &out, int &lbl_counter, int &temp_counter,
            std::unordered_map<label, label> &label_map, int &call_counter);
label check_map_and_rename_label(std::unordered_map<label, label> &map, label &lbl, int &global_counter);
std::vector<i>
regs_to_args(std::vector<t> &args);
void emit_call(std::vector<i> &new_function_instructions, callee_type &c, int64_t num_args);
/*
atomic tiles
*/
s atomic_assign_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_load_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_store_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_Aop_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_CmpOp_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_return_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_label_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_br_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_cond_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s atomic_call_tile(std::unique_ptr<CallNode> &node, TileCtx &ctx);
/*
larger tiles
*/
s self_op_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s self_op_commutative_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s increment_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s decrement_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s mul_by_pow2_self_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s mul_by_pow2_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s at_addr_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s cjump_fusion_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s load_with_offset_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
s store_with_offset_tile(std::unique_ptr<Inner> &node, TileCtx &ctx);
std::optional<s> match_large_tile(Op op, std::unique_ptr<Inner> &node, TileCtx &ctx);

} // namespace L3