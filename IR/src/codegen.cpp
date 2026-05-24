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

static L3::var compute_elem_addr(L3::var arr, std::vector<L3::t> &indices,
                                 std::vector<L3::i> &out,
                                 std::function<L3::var()> &fresh_var) {
  int n = indices.size();

  // ADDR_M <- A + 16 / M_ <- load ADDR_M / M <- M_ >> 1
  //      ADDR_N <- A + 24 / N_ <- load ADDR_N / N <- N_ >> 1
  std::vector<L3::var> dims;
  for (int i = 1; i < n; i++) { // for all dimensions
    L3::var dim_addr = fresh_var(), dim_enc = fresh_var(), dim_raw = fresh_var();
    out.push_back(L3::AopL3{L3::w{dim_addr}, L3::t{arr}, L3::aop::add, L3::t{L3::N{(int64_t)(i + 1) * 8}}}); // calculate address
    out.push_back(L3::LoadL3{dim_enc, dim_addr});                                                            // load the encoded dimension
    out.push_back(L3::AopL3{L3::w{dim_raw}, L3::t{dim_enc}, L3::aop::rshift, L3::t{L3::N{1}}});              // decode
    dims.push_back(dim_raw);
  }

  // newVar1 <- (i * N)
  // newVar2 <- (k * M * N)
  // newVar3 <- (k*M*N
  // index <- (k*M*N) + (i*N) + j
  L3::var idx_var = fresh_var();
  out.push_back(L3::Assign{L3::w{idx_var}, L3::s{L3::N{0}}});
  if (n > 1) {
    L3::var stride = dims[n - 2];
    for (int j = n - 2; j >= 0; j--) {
      L3::var contrib = fresh_var(), new_lin = fresh_var();
      out.push_back(L3::AopL3{L3::w{contrib}, indices[j], L3::aop::mul, L3::t{stride}});
      out.push_back(L3::AopL3{L3::w{new_lin}, L3::t{idx_var}, L3::aop::add, L3::t{contrib}});
      idx_var = new_lin;
      if (j > 0) {
        L3::var new_stride = fresh_var();
        out.push_back(L3::AopL3{L3::w{new_stride}, L3::t{dims[j - 1]}, L3::aop::mul, L3::t{stride}});
        stride = new_stride;
      }
    }
  }

  // add inner idx
  // index <- newVar3 + j
  L3::var flat = fresh_var();
  out.push_back(L3::AopL3{L3::w{flat}, L3::t{idx_var}, L3::aop::add, indices[n - 1]});

  //   Matrix M x N:
  // Offset for all: B = 8 + (2 * 8)
  L3::var byte_off = fresh_var();
  out.push_back(L3::AopL3{L3::w{byte_off}, L3::t{flat}, L3::aop::mul, L3::t{L3::N{8}}});
  out.push_back(L3::AopL3{L3::w{byte_off}, L3::t{byte_off}, L3::aop::add,
                          L3::t{L3::N{(int64_t)(n + 1) * 8}}});

  L3::var addr = fresh_var();
  out.push_back(L3::AopL3{L3::w{addr}, L3::t{arr}, L3::aop::add, L3::t{byte_off}});
  return addr;
}
std::vector<L3::i> NewArray_to_l3(IR::NewArray &na, std::function<L3::var()> &fresh_var) {
  std::vector<L3::i> out;
  int n = na.dims.size();

  // decode by right shifting by 1...
  L3::var prod = fresh_var();
  out.push_back(L3::Assign{L3::w{prod}, L3::s{L3::N{1}}});
  for (auto &dim : na.dims) {
    L3::var dec = fresh_var();
    L3::var new_prod = fresh_var();
    out.push_back(L3::AopL3{L3::w{dec}, dim, L3::aop::rshift, L3::t{L3::N{1}}});      // assign the decoded value to dec fresh var
    out.push_back(L3::AopL3{L3::w{new_prod}, L3::t{prod}, L3::aop::mul, L3::t{dec}}); // multiply times prod
    prod = new_prod;
  }

  // encode(product + n)
  L3::var enc = fresh_var();
  out.push_back(L3::AopL3{L3::w{enc}, L3::t{prod}, L3::aop::add, L3::t{L3::N{n}}});
  out.push_back(L3::AopL3{L3::w{enc}, L3::t{enc}, L3::aop::lshift, L3::t{L3::N{1}}});
  out.push_back(L3::AopL3{L3::w{enc}, L3::t{enc}, L3::aop::add, L3::t{L3::N{1}}});

  // %a <- call allocate(%v0, 1)
  out.push_back(L3::CallAssignL3{na.dest,
                                 L3::CallL3{L3::CallAllocate{}, {L3::t{enc}, L3::t{L3::N{1}}}}});
  // store all of the dimensions
  // %v2 <- %a + 8
  // store %v2 <- %p1 ...
  for (int i = 0; i < n; i++) {
    L3::var ai = fresh_var();
    out.push_back(L3::AopL3{L3::w{ai}, L3::t{na.dest}, L3::aop::add,
                            L3::t{L3::N{(int64_t)(i + 1) * 8}}});
    out.push_back(L3::StoreL3{ai, t_to_s(na.dims[i])});
  }
  return out;
}

std::vector<L3::i> Length_to_l3(IR::Length &len, std::function<L3::var()> &fresh_var) {
  std::vector<L3::i> out;
  if (!len.dim) {
    // %l1 <- %l1 << 1
    // %l1 <- %l1 + 1
    out.push_back(L3::LoadL3{len.dest, len.src});
    out.push_back(L3::AopL3{L3::w{len.dest}, L3::t{len.dest}, L3::aop::lshift, L3::t{L3::N{1}}});
    out.push_back(L3::AopL3{L3::w{len.dest}, L3::t{len.dest}, L3::aop::add, L3::t{L3::N{1}}});
    return out;
  }
  L3::var addr = fresh_var();
  // cond the dimension is a number do static rule offset = (k+1)*8
  if (std::holds_alternative<L3::N>(*len.dim)) {
    int64_t k = std::get<L3::N>(*len.dim).value;
    out.push_back(L3::AopL3{L3::w{addr}, L3::t{len.src}, L3::aop::add,
                            L3::t{L3::N{(k + 1) * 8}}});
  } else {
    // O.W offset = dim*8 + 8
    L3::var off = fresh_var();
    out.push_back(L3::AopL3{L3::w{off}, *len.dim, L3::aop::mul, L3::t{L3::N{8}}});
    out.push_back(L3::AopL3{L3::w{off}, L3::t{off}, L3::aop::add, L3::t{L3::N{8}}});
    out.push_back(L3::AopL3{L3::w{addr}, L3::t{len.src}, L3::aop::add, L3::t{off}});
  }
  out.push_back(L3::LoadL3{len.dest, addr});
  return out;
}

std::vector<L3::i> ArrayRead_to_l3(IR::ArrayRead &ar, std::function<L3::var()> &fresh_var) {
  std::vector<L3::i> out;
  L3::var addr = compute_elem_addr(ar.src, ar.indices, out, fresh_var);
  out.push_back(L3::LoadL3{ar.dest, addr});
  return out;
}

std::vector<L3::i> ArrayWrite_to_l3(IR::ArrayWrite &aw, std::function<L3::var()> &fresh_var) {
  std::vector<L3::i> out;
  L3::var addr = compute_elem_addr(aw.dest, aw.indices, out, fresh_var);
  out.push_back(L3::StoreL3{addr, aw.src});
  return out;
}
L3::i NewTuple_to_l3(IR::NewTuple &nt) {
  // %t <- call allocate(7, 1)
  return L3::CallAssignL3{nt.dest,
                          L3::CallL3{L3::CallAllocate{}, {nt.size, L3::t{L3::N{1}}}}};
}

L3::fl3 fIR_to_fl3(fIR &f) {
  int temp_counter = 0;
  std::function<L3::var()> fresh_var = [&]() { return L3::var{"__IR_tmp_" + std::to_string(temp_counter++)}; }; // closure passed to the helper functions

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
        } else if constexpr (std::is_same_v<T, ArrayRead>) {
          auto res = ArrayRead_to_l3(inst, fresh_var); // finally some functional programming haha. Functions as first class citizens let's goo
          new_function.instructions.insert(new_function.instructions.end(), res.begin(), res.end());
        } else if constexpr (std::is_same_v<T, ArrayWrite>) {
          auto res = ArrayWrite_to_l3(inst, fresh_var);
          new_function.instructions.insert(new_function.instructions.end(), res.begin(), res.end());
        } else if constexpr (std::is_same_v<T, Length>) {
          auto res = Length_to_l3(inst, fresh_var);
          new_function.instructions.insert(new_function.instructions.end(), res.begin(), res.end());
        } else if constexpr (std::is_same_v<T, NewArray>) {
          auto res = NewArray_to_l3(inst, fresh_var);
          new_function.instructions.insert(new_function.instructions.end(), res.begin(), res.end());
        } else if constexpr (std::is_same_v<T, NewTuple>) {
          new_function.instructions.push_back(NewTuple_to_l3(inst));
        }
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