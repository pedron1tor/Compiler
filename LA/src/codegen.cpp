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
bool is_declaration(LA::i &inst, std::vector<IR::TypeVar> &variable_accumulator) {
  return std::visit([&](auto &ins) -> bool {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, LA::TypeVar>) {
      IR::TypeVar type_var = IR::TypeVar{ins.var_type, L3::var{ins.var_declared}};
      variable_accumulator.push_back(type_var);
      return true;
    } else {
      return false;
    }
  },
                    inst);
};

static IR::i LA_to_IRinst(LA::i &inst, const std::set<std::string> &code_vars) {
  return std::visit([&](auto &ins) -> IR::i {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, LA::Assign>)
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
void variable_declaration(std::vector<IR::TypeVar> &all_var_accumulator, std::vector<IR::bb> &blocks) {
  if (blocks.size() > 0) {
    for (auto typed_var : all_var_accumulator) {
      blocks[0].instructions.insert(blocks[0].instructions.begin(), typed_var); // add all the variable declarations
    }
    for (auto typed_var : all_var_accumulator) {
      int value = std::visit([&](auto &ins) -> int {
        using T = std::decay_t<decltype(ins)>;
        if constexpr (std::is_same_v<T, IR::t_int64>)
          return 1;
        else
          return 0;
      },
                             typed_var.var_type);
      blocks[0].instructions.insert(blocks[0].instructions.begin(), IR::i{L3::Assign{typed_var.var_declared, L3::N{value}}});
      // assign the default values to the variables
    }
  }
};

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
  std::vector<IR::TypeVar> all_var_accumulator;
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
      bool is_decl = is_declaration(instr, all_var_accumulator);
      if (!is_term) {
        if (!is_decl) {
          block.instructions.push_back(LA_to_IRinst(instr, code_vars));
        }

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
  // variable declaration
  variable_declaration(all_var_accumulator, blocks);
  IR::fIR result;
  result.f_type = f.f_type;
  result.name = L3::l{"@" + f.name};
  result.parameters = ir_pars;
  result.blocks = blocks;
  return result;
}
L3::N encode_N(L3::N num) {
  return L3::N{((2 * num.value) + 1)};
};
L3::N decode_N(L3::N num) {
  return L3::N{(num.value >> 1)};
}

L3::t encode_t(L3::t orig_value) {
  return std::visit([&](auto &ins) -> L3::t {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, L3::N>)
      return encode_N(ins);
    else
      return ins;
  },
                    orig_value);
};
L3::s encode_s(L3::s orig_value) {
  return std::visit([&](auto &ins) -> L3::s {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, L3::N>)
      return encode_N(ins);
    else
      return ins;
  },
                    orig_value);
};
template <typename T, typename F> // why is there no map in cpp???? .......
auto map(const std::vector<T> &vec, F f) {
  std::vector<decltype(f(vec[0]))> result;
  result.reserve(vec.size());
  for (const auto &x : vec) {
    result.push_back(f(x));
  }
  return result;
}
std::optional<L3::var> toEncode(IR::i &inst) {
  return std::visit([&](auto &ins) -> std::optional<L3::var> {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, IR::BinOp>)
      return ins.dest;
    else
      return std::nullopt;
  },
                    inst);
};
std::optional<std::vector<L3::t>> toDecode(IR::i &inst) {
  return std::visit([&](auto &ins) -> std::optional<std::vector<L3::t>> {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, IR::BinOp>)
      return std::vector<L3::t>{ins.left, ins.right};
    else if constexpr (std::is_same_v<T, IR::ArrayWrite>)
      return ins.indices;
    else if constexpr (std::is_same_v<T, IR::ArrayRead>)
      return ins.indices;
    else if constexpr (std::is_same_v<T, IR::Length>)
      return ins.dim.has_value() ? std::optional<std::vector<L3::t>>{std::vector<L3::t>{*ins.dim}} : std::nullopt;
    else if constexpr (std::is_same_v<T, IR::BrCond>)
      return std::vector<L3::t>{ins.cond};
    else
      return std::nullopt;
  },
                    inst);
};

IR::i substitute(IR::i &inst, std::vector<L3::var> identifiers) {
  auto var_to_t = [](const L3::var &v) -> L3::t { return v; };
  return std::visit([&](auto &ins) -> IR::i {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, IR::BinOp>)
      return IR::BinOp{ins.dest, L3::t{identifiers[0]}, ins.operation, L3::t{identifiers[1]}};
    else if constexpr (std::is_same_v<T, IR::ArrayWrite>)
      return IR::ArrayWrite{ins.dest, map(identifiers, var_to_t), ins.src};
    else if constexpr (std::is_same_v<T, IR::ArrayRead>)
      return IR::ArrayRead{ins.dest, ins.src, map(identifiers, var_to_t)};
    else if constexpr (std::is_same_v<T, IR::Length>)
      return IR::Length{ins.dest, ins.src, L3::t{identifiers[0]}};
    else
      return ins;
  },
                    inst);
};

void value_encoding(std::vector<IR::fIR> &function_list) {
  // Encode all constants not used for array/tuple indices
  // and 2nd parameter of length
  int counter = 0;
  // gensym helper to create labels for the blocks. L cpp for not having it builtin...
  auto gensym = [&counter]() -> L3::var {
    return L3::var{":_label_encoding" + std::to_string(counter++)};
  };
  std::vector<IR::fIR> new_function_list;
  for (auto &fn : function_list) {
    std::vector<IR::i> declarations_first_bb;
    for (auto &block : fn.blocks) {
      int bound = block.instructions.size();
      std::vector<IR::i> block_instructions;

      for (int i = 0; i < bound; i++) {
        block_instructions.push_back(std::visit([&](auto &ins) -> IR::i {
          using T = std::decay_t<decltype(ins)>;
          if constexpr (std::is_same_v<T, L3::Assign>)

            return L3::Assign{ins.dest, encode_s(ins.src)};
          else if constexpr (std::is_same_v<T, IR::BinOp>)
            return IR::BinOp{ins.dest, encode_t(ins.left), ins.operation, encode_t(ins.right)};
          else if constexpr (std::is_same_v<T, L3::CallAssignL3>) {
            return L3::CallAssignL3{ins.dest, L3::CallL3{
                                                  ins.callexpr.callee, map(ins.callexpr.args, encode_t)}};

          } else if constexpr (std::is_same_v<T, L3::CallL3>) {
            return L3::CallL3{
                ins.callee, map(ins.args, encode_t)};
          } else
            return ins;
        },
                                                block.instructions[i]));
        std::optional<std::vector<L3::t>> things_to_decode = toDecode(block.instructions[i]);
        if (things_to_decode.has_value()) {
          std::vector<L3::var> identifiers;
          for (auto thing_to_decode : things_to_decode.value()) {
            L3::var var_identifier = gensym();
            declarations_first_bb.push_back(IR::i{IR::TypeVar{IR::type{IR::t_int64{}}, var_identifier}});
            block_instructions.push_back(IR::i{IR::BinOp{var_identifier, encode_t(thing_to_decode), IR::op::rshift, L3::N{1}}});
            identifiers.push_back(var_identifier);
          }
          block_instructions.push_back(substitute(block.instructions[i], identifiers));
        }
        std::optional<L3::var> variable_to_encode = toEncode(block.instructions[i]);
        if (variable_to_encode.has_value()) {
          // encode the variable
          block_instructions.push_back(IR::BinOp{
              variable_to_encode.value(), variable_to_encode.value(), IR::op::lshift, L3::N{1}});
          block_instructions.push_back(IR::BinOp{
              variable_to_encode.value(), variable_to_encode.value(), IR::op::add, L3::N{1}});
        };
      }
      // decode BrCond condition and encode ReturnT value
      block.terminal = std::visit([&](auto &te) -> IR::te {
        using T = std::decay_t<decltype(te)>;
        if constexpr (std::is_same_v<T, IR::BrCond>) {
          L3::var decoded = gensym();
          declarations_first_bb.push_back(IR::i{IR::TypeVar{IR::type{IR::t_int64{}}, decoded}});
          block_instructions.push_back(IR::i{IR::BinOp{decoded, encode_t(te.cond), IR::op::rshift, L3::N{1}}});
          return IR::BrCond{L3::t{decoded}, te.true_label, te.false_label};
        } else if constexpr (std::is_same_v<T, L3::ReturnT>)
          return L3::ReturnT{encode_t(te.return_thing)};
        else
          return te;
      },
                                  block.terminal);
      block.instructions = block_instructions;
    }
    fn.blocks[0].instructions.insert(fn.blocks[0].instructions.begin(), declarations_first_bb.begin(), declarations_first_bb.end());
  };
};

IR::pIR pLA_to_pIR(LA::pLA &p) {
  std::vector<IR::fIR> function_list;

  for (auto &fn : p.functions) {
    // bb generation and variable declaration done in fLA_to_fIR
    function_list.push_back(fLA_to_fIR(fn));
  }
  // value encoding
  value_encoding(function_list);
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