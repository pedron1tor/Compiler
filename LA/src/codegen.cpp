#include "LA.h"
#include "parser.h"
#include "parsing/primitives.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <unordered_map>
#include <utils.h>

namespace LA {

bool is_terminator(LA::i &instruction) {
  return std::visit([&](auto &ins) {
    // no gotos in LA
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, BrCond>)
      return true;
    else if constexpr (std::is_same_v<T, L3::Return>)
      return true;
    else if constexpr (std::is_same_v<T, LA::ReturnT>)
      return true;
    else
      return false;
  },
                    instruction);
};
bool f_type_void(LA::fLA &f) {
  return std::visit([&](auto &func) {
    using T = std::decay_t<decltype(func)>;
    if constexpr (std::is_same_v<T, IR::t_void>)
      return true;
    else
      return false;
  },
                    f.f_type);
};
bool inst_is_labelinst(LA::i &instr) {
  return std::visit([&](auto &ins) {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, L3::LabelInst>)
      return true;
    else
      return false;
  },
                    instr);
};

static IR::i LA_to_IRinst(LA::i &inst, const std::set<std::string> &code_vars) {
  return std::visit([&](auto &ins) -> IR::i {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, LA::TypeVar>)
      return IR::TypeVar{ins.var_type, L3::var{ins.var_declared}};
    else if constexpr (std::is_same_v<T, LA::Assign>)
      return L3::Assign{L3::w{L3::var{ins.dest}}, la_s_to_l3_s(ins.src)};
    else if constexpr (std::is_same_v<T, LA::BinOp>)
      return IR::BinOp{L3::var{ins.dest}, la_t_to_l3_t(ins.left), ins.operation, la_t_to_l3_t(ins.right)};
    else if constexpr (std::is_same_v<T, LA::ArrayRead>)
      return IR::ArrayRead{L3::var{ins.dest}, L3::var{ins.src}, la_t_vec_to_l3_t_vec(ins.indices)};
    else if constexpr (std::is_same_v<T, LA::ArrayWrite>)
      return IR::ArrayWrite{L3::var{ins.dest}, la_t_vec_to_l3_t_vec(ins.indices), la_s_to_l3_s(ins.src)};
    else if constexpr (std::is_same_v<T, LA::Length>)
      return IR::Length{L3::var{ins.dest}, L3::var{ins.src},
                        ins.dim ? std::optional<L3::t>{la_t_to_l3_t(*ins.dim)} : std::nullopt};
    else if constexpr (std::is_same_v<T, LA::NewArray>)
      return IR::NewArray{L3::var{ins.dest}, la_t_vec_to_l3_t_vec(ins.dims)};
    else if constexpr (std::is_same_v<T, LA::NewTuple>)
      return IR::NewTuple{L3::var{ins.dest}, la_t_to_l3_t(ins.size)};
    else if constexpr (std::is_same_v<T, LA::CallAssignArbitrary>)
      return L3::CallAssignL3{L3::var{ins.dest},
                              L3::CallL3{str_to_callee(ins.callexpr.callee, code_vars),
                                         la_t_vec_to_l3_t_vec(ins.callexpr.args)}};
    else if constexpr (std::is_same_v<T, LA::CallArbitrary>)
      return L3::CallL3{str_to_callee(ins.callee, code_vars), la_t_vec_to_l3_t_vec(ins.args)};
    else
      return IR::TypeVar{IR::type{IR::t_void{}}, L3::var{""}};
  },
                    inst);
}

IR::fIR fLA_to_fIR(LA::fLA &f) {
  // collect code variables to assert if function call is valid later, or not.
  std::set<std::string> code_vars;
  for (auto &tv : f.parameters.input_variables)
    if (std::holds_alternative<IR::t_code>(tv.var_type))
      code_vars.insert(tv.var_declared);
  for (auto &instr : f.instructions)
    if (auto *tv = std::get_if<LA::TypeVar>(&instr))
      if (std::holds_alternative<IR::t_code>(tv->var_type))
        code_vars.insert(tv->var_declared);

  int counter = 0;
  // gensym helper to create labels for the blocks. L cpp for not having it builtin...
  auto gensym = [&counter]() -> L3::label {
    return L3::label{":_label_" + std::to_string(counter++)};
  };

  bool startBB = true;
  std::vector<IR::bb> blocks; // list of blocks
  IR::bb block;

  for (auto &instr : f.instructions) {
    if (inst_is_labelinst(instr)) {
      auto &lbl = std::get<L3::LabelInst>(instr).lbl; // get the label obj from LabelInst
      if (!startBB) {
        block.terminal = IR::te{L3::Goto{lbl}}; // if we've encountered a label but it isn't the start of a bb, then add a goto to the block
        blocks.push_back(block);
        block = IR::bb{}; // then start new bb
      }
      block.lbl = lbl; // add the label to the block
      startBB = false;
    } else { // if the instruction wasn't a label, then gensym a new label to a new block
      if (startBB) {
        block = IR::bb{};
        block.lbl = gensym();
        startBB = false;
      }
      bool is_brlbl = std::holds_alternative<L3::brlabel>(instr);
      bool is_term = is_terminator(instr) || is_brlbl; // compute whether it's a terminator or brblabel
      if (!is_term) {
        block.instructions.push_back(LA_to_IRinst(instr, code_vars));
      } else {
        block.terminal = LAterm_to_IRterm(instr); // transform into IR instruction forms
        blocks.push_back(block);
        block = IR::bb{};
        startBB = true;
      }
    }
  }
  // return at the bottom of void function, else add returnt for completeness, although startBB = false at the end shouldn't happen if program is correct
  if (!startBB) {
    block.terminal = f_type_void(f) ? IR::te{L3::Return{}} : IR::te{L3::ReturnT{L3::t{L3::N{0}}}};
    blocks.push_back(block);
  }

  IR::pars ir_pars;
  // wrap the strings into variables
  for (auto &tv : f.parameters.input_variables)
    ir_pars.input_variables.push_back(IR::TypeVar{tv.var_type, L3::var{tv.var_declared}});

  IR::fIR result;
  result.f_type = f.f_type;
  result.name = L3::l{"@" + f.name};
  result.parameters = ir_pars;
  result.blocks = blocks;
  return result;
}

IR::pIR pLA_to_pIR(LA::pLA &p) {
  std::vector<IR::fIR> function_list;

  for (auto &fn : p.functions) {
    // bb generation and variable declaration done in fLA_to_fIR
    function_list.push_back(fLA_to_fIR(fn));
  }
  // value encoding
  // allocation and bounds check
  return IR::pIR{
      function_list};
};

void generate_code(pLA &p) {
  IR::pIR transformed_p;
  transformed_p = pLA_to_pIR(p);
  std::ofstream outputFile;
  outputFile.open("prog.IR");

  // // build the IR code for each of the funtions inside of the ast/program
  for (auto &fn : transformed_p.functions) {
    print_functionIR(fn, true, &outputFile);
  };

  outputFile.close();
}
} // namespace LA