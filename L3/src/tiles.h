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
s atomic_assign_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                     int &lbl_counter, int &temp_counter,
                     std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_load_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                   int &lbl_counter, int &temp_counter,
                   std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_store_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                    int &lbl_counter, int &temp_counter,
                    std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_Aop_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                  int &lbl_counter, int &temp_counter,
                  std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_CmpOp_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                    int &lbl_counter, int &temp_counter,
                    std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_return_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                     int &lbl_counter, int &temp_counter,
                     std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_label_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                    int &lbl_counter, int &temp_counter,
                    std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_br_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                 int &lbl_counter, int &temp_counter,
                 std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_cond_tile(std::unique_ptr<Inner> &node, std::vector<i> &out,
                   int &lbl_counter, int &temp_counter,
                   std::unordered_map<label, label> &label_map, int &call_counter);
s atomic_call_tile(std::unique_ptr<CallNode> &node, std::vector<i> &out,
                   int &lbl_counter, int &temp_counter,
                   std::unordered_map<label, label> &label_map, int &call_counter);
} // namespace L3