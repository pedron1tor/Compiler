#include <algorithm>
#include <assert.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sched.h>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
#include <tao/pegtl/contrib/trace.hpp>

#include <IR.h>
#include <parser.h>

#include "parsing/instructions.h"
#include "parsing/primitives.h"
#include "parsing/registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace IR {

/*
 * Grammar rules definitions from now on.
 */

struct parse_function_name : pegtl::seq<pegtl::one<'@'>, parse_name> {
};
// pars ::= type var | type var (, type var)* |
struct parse_pars_type : parse_type {};
struct parse_pars_var : parse_variable {};
struct parse_pars : pegtl::seq<parse_pars_type, parse_spaces, parse_pars_var> {};
struct parse_vars : pegtl::seq<
                        pegtl::one<'('>, parse_seps,
                        pegtl::opt<pegtl::seq<
                            parse_pars,
                            pegtl::star<pegtl::seq<parse_seps, parse_comma, parse_seps, parse_pars>>>>,
                        parse_seps, pegtl::one<')'>> {};
struct parse_function_rule : pegtl::seq<parse_seps,
                                        parse_define,
                                        parse_seps,
                                        parse_T,
                                        parse_seps,
                                        parse_function_name,
                                        parse_seps,
                                        parse_vars,
                                        parse_seps,
                                        pegtl::one<'{'>,
                                        parse_instructions,
                                        parse_seps,
                                        pegtl::one<'}'>> {
};
struct parse_functions_rule : pegtl::seq<
                                  pegtl::plus<parse_function_rule>> {
};
struct parse_entry_point_rule : pegtl::seq<
                                    parse_seps,
                                    parse_functions_rule,
                                    parse_seps> {
};

struct grammar : pegtl::seq<
                     parse_entry_point_rule, pegtl::eof> {
};

/*
 * templates
 */
template <>
struct action<parse_function_name> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    fIR newF;
    newF.name.name = in.string();
    newF.f_type = acc.parsed_types.back();
    acc.parsed_types.pop_back();
    acc.program.functions.push_back(newF);
  }
};

template <>
struct action<parse_pars_var> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_vars.push_back(L3::var{remove_perc(in.string())}); }
};

template <>
struct action<parse_pars> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto var_name = acc.parsed_vars.back();
    acc.parsed_vars.pop_back();
    auto var_type = acc.parsed_types.back();
    acc.parsed_types.pop_back();
    acc.program.functions.back().parameters.input_variables.push_back(TypeVar{var_type, var_name});
  }
};

pIR parse_file(char *filename) {
  /*
   * Check the grammar for some possible issues.
   */
  if (pegtl::analyze<grammar>() != 0) {
    std::cerr << "There are problems with the grammar" << std::endl;
    exit(1);
  }

  /*
   * Parse.
   */
  file_input<> fileInput(filename);
  // run to debug program...
  // pegtl::standard_trace<grammar>(fileInput);
  GlobalAccumulator state;
  parse<grammar, action>(fileInput, state);
  return state.program;
}

void print_functionIR(fIR &fn) {
  std::cout << "define " << type_to_str(fn.f_type) << fn.name.name << " (";
  bool first = true;
  for (auto &tv : fn.parameters.input_variables) {
    if (!first)
      std::cout << ", ";
    first = false;
    std::cout << add_perc(tv.var_declared.name);
  }
  std::cout << ") {\n";
  for (auto &block : fn.blocks) {
    std::cout << "\t";
    std::cout << block.lbl.name << "\n";
    for (auto &instr : block.instructions) {
      std::cout << "\t";
      std::visit([&](auto &ins) {
          using T = std::decay_t<decltype(ins)>;
          if constexpr (std::is_same_v<T, L3::Assign>)
            std::cout << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
          else if constexpr (std::is_same_v<T, BinOp>)
            std::cout << add_perc(ins.dest.name) << " <- " << t_to_str(ins.left) << " " << op_to_str(ins.operation) << " " << t_to_str(ins.right);
          else if constexpr (std::is_same_v<T, L3::CallL3>)
            std::cout << "call " << callee_to_str(ins.callee) << call_args_to_str(ins.args);
          else if constexpr (std::is_same_v<T, L3::CallAssignL3>)
            std::cout << add_perc(ins.dest.name) << " <- call " << callee_to_str(ins.callexpr.callee) << call_args_to_str(ins.callexpr.args);
          else if constexpr (std::is_same_v<T, L3::CallPrint>)
            std::cout << "call print()";
          else if constexpr (std::is_same_v<T, L3::CallInput>)
            std::cout << "call input()";
          else if constexpr (std::is_same_v<T, L3::CallTupleError>)
            std::cout << "call tuple-error()";
          else if constexpr (std::is_same_v<T, L3::CallTensorError>)
            std::cout << "call tensor-error()";
          else if constexpr (std::is_same_v<T, ArrayRead>)
            std::cout << add_perc(ins.dest.name) << " <- " << add_perc(ins.src.name) << indices_to_str(ins.indices);
          else if constexpr (std::is_same_v<T, ArrayWrite>)
            std::cout << add_perc(ins.dest.name) << indices_to_str(ins.indices) <<  " <- " << s_to_str(ins.src) ;
          else if constexpr (std::is_same_v<T, Length>)
            std::cout << add_perc(ins.dest.name) << " <- length " << add_perc(ins.src.name);
          else if constexpr (std::is_same_v<T, NewArray>)
            std::cout << add_perc(ins.dest.name) << " <- new Array(" << dims_to_str(ins.dims) << ")";
          else if constexpr (std::is_same_v<T, NewTuple>)
            std::cout << add_perc(ins.dest.name) << " <- new Tuple(" << t_to_str(ins.size)<< ")";
          else if constexpr (std::is_same_v<T, TypeVar>)
            std::cout << type_to_str(ins.var_type) << add_perc(ins.var_declared.name); },
                 instr);
      std::cout << "\n";
    }
    std::cout << "\t";
    std::visit([&](auto &te_inst) {
        using T = std::decay_t<decltype(te_inst)>;
        if constexpr (std::is_same_v<T, L3::Return>)
          std::cout << "return";
        else if constexpr (std::is_same_v<T, L3::ReturnT>)
          std::cout << "return " << t_to_str(te_inst.return_thing);
        else if constexpr (std::is_same_v<T, L3::Goto>)
          std::cout << "br " << te_inst.target.name;
        else if constexpr (std::is_same_v<T, BrCond>)
          std::cout << "br " << t_to_str(te_inst.cond) << " " << te_inst.true_label.name << " " << te_inst.false_label.name; },
               block.terminal);
    std::cout << "\n";
  }
  std::cout << "}\n";
};

L3::i BinOp_to_l3op(IR::BinOp &b) {
  L3::w dest{b.dest};
  switch (b.operation) {
  case op::add:    return L3::AopL3{dest, b.left, L3::aop::add,    b.right};
  case op::sub:    return L3::AopL3{dest, b.left, L3::aop::sub,    b.right};
  case op::mul:    return L3::AopL3{dest, b.left, L3::aop::mul,    b.right};
  case op::and_:   return L3::AopL3{dest, b.left, L3::aop::and_,   b.right};
  case op::lshift: return L3::AopL3{dest, b.left, L3::aop::lshift, b.right};
  case op::rshift: return L3::AopL3{dest, b.left, L3::aop::rshift, b.right};
  case op::lt:     return L3::Compare{dest, b.left, L3::cmp::lt,  b.right};
  case op::gt:     return L3::Compare{dest, b.left, L3::cmp::gt,  b.right};
  case op::le:     return L3::Compare{dest, b.left, L3::cmp::le,  b.right};
  case op::gte:    return L3::Compare{dest, b.left, L3::cmp::gte, b.right};
  default:         return L3::Compare{dest, b.left, L3::cmp::eq,  b.right};
  }
}

void print_functionl3(L3::fl3 &fn, bool generator, std::ofstream *outputFile) {
  std::ostream *outp = (generator && outputFile) ? static_cast<std::ostream *>(outputFile) : &std::cout;
  std::ostream &out = *outp;
  out << "define " << fn.name.name;
  // print variables
  out << " (";
  bool first = true;
  for (L3::var &variable : fn.vars) {
    if (first) {
      first = false;
    } else {
      out << ", ";
    }
    out << add_perc(variable.name);
  }
  out << ")";
  out << "{\n";
  for (auto &instr : fn.instructions) {
    out << "\t";
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, L3::Return>)
        out << "return";
      else if constexpr (std::is_same_v<T, L3::ReturnT>)
        out << "return " << t_to_str(ins.return_thing); 
      else if constexpr (std::is_same_v<T, L3::Assign>)
        out << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::AopInst>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::LabelInst>)
        out << ins.lbl.name;
     
    
      else if constexpr (std::is_same_v<T, L3::Compare>)
        out << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, L3::StackArg>)
        out << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      else if constexpr (std::is_same_v<T, L3::LoadAddr>)
        out << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale;
      else if constexpr (std::is_same_v<T, L3::AopL3>)
        out << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, L3::StoreL3>)
        out << "store " << add_perc(ins.dest.name) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::LoadL3>)
        out << add_perc(ins.dest.name) << " <- load " << add_perc(ins.src.name);
      else if constexpr (std::is_same_v<T, L3::brlabel>)
        out << "br " << ins.lbl.name;
      else if constexpr (std::is_same_v<T, L3::brtlabel>)
        out << "br " << t_to_str(ins.cond) << " " << ins.lbl.name;
      else if constexpr (std::is_same_v<T, L3::CallL3>)
        out << "call " << callee_to_str(ins.callee) << call_args_to_str(ins.args);
      else if constexpr (std::is_same_v<T, L3::CallAssignL3>)
        out << add_perc(ins.dest.name) << " <- call " << callee_to_str(ins.callexpr.callee) << call_args_to_str(ins.callexpr.args); },
               instr);
    out << "\n";
  }
  out << "}\n";
};

void print_functionl2(L3::fl2 &fn, bool generator, std::ofstream *outputFile) {
  std::ostream *outp = (generator && outputFile) ? static_cast<std::ostream *>(outputFile) : &std::cout;
  std::ostream &out = *outp;
  out << "(" << fn.name.name << "\n";
  out << "\t" << fn.num_args << "\n";
  for (auto &instr : fn.instructions) {
    out << "\t";
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, L3::Return>)
        out << "return";
      else if constexpr (std::is_same_v<T, L3::Assign>)
        out << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::AopInst>)
        out << w_to_str(ins.dest) << " " << aop_to_str_l2(ins.op) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::SopReg>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << t_to_str(ins.shift);
      else if constexpr (std::is_same_v<T, L3::SopNum>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << ins.shift.value;
      else if constexpr (std::is_same_v<T, L3::LabelInst>)
        out << ins.lbl.name;
      else if constexpr (std::is_same_v<T, L3::CallPrint>)
        out << "call print 1";
      else if constexpr (std::is_same_v<T, L3::CallInput>)
        out << "call input 0";
      else if constexpr (std::is_same_v<T, L3::CallAllocate>)
        out << "call allocate 2";
      else if constexpr (std::is_same_v<T, L3::CallTupleError>)
        out << "call tuple-error 3";
      else if constexpr (std::is_same_v<T, L3::Increment>)
        out << w_to_str(ins.reg) << "++";
      else if constexpr (std::is_same_v<T, L3::Decrement>)
        out << w_to_str(ins.reg) << "--";
      else if constexpr (std::is_same_v<T, L3::CallTensorError>)
        out << "call tensor-error " << ins.F;
      else if constexpr (std::is_same_v<T, L3::Compare>)
        out << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, L3::Cjump>)
        out << "cjump " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right) << " " << ins.target.name;
      else if constexpr (std::is_same_v<T, L3::Goto>)
        out << "goto " << ins.target.name;
      else if constexpr (std::is_same_v<T, L3::MemRead>)
        out << w_to_str(ins.dest) << " <- mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, L3::MemWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::MemAopRead>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.operation) << " mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, L3::MemAopWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " " << aop_to_str(ins.operation) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::StackArg>)
        out << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      else if constexpr (std::is_same_v<T, L3::Call>)
        out << "call " << std::visit([&](auto &v) -> std::string {
          using U = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<U, L3::r>)
            return r_to_str(v);
          else if (std::is_same_v<U, L3::var>)
            return add_perc(v.name);
          else
            return v.name;
        },
                                           ins.callee)
                  << " " << ins.num_args.value;
      else if constexpr (std::is_same_v<T, L3::LoadAddr>)
        out << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale; },
               instr);
    out << "\n";
  }
  out << ")\n";
};
void print_functionl1(L3::fl1 &fn, bool generator, std::ofstream *outputFile) {
  std::ostream *outp = (generator && outputFile) ? static_cast<std::ostream *>(outputFile) : &std::cout;
  std::ostream &out = *outp;
  out << "(" << fn.name.name << "\n";
  out << "    " << fn.num_args << " " << fn.num_locals << "\n";
  for (auto &instr : fn.instructions) {
    out << "\t";
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, L3::Return>)
        out << "return";
      else if constexpr (std::is_same_v<T, L3::Assign>)
        out << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::AopInst>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::SopReg>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << t_to_str(ins.shift);
      else if constexpr (std::is_same_v<T, L3::SopNum>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << ins.shift.value;
      else if constexpr (std::is_same_v<T, L3::LabelInst>)
        out << ins.lbl.name;
      else if constexpr (std::is_same_v<T, L3::CallPrint>)
        out << "call print 1";
      else if constexpr (std::is_same_v<T, L3::CallInput>)
        out << "call input 0";
      else if constexpr (std::is_same_v<T, L3::CallAllocate>)
        out << "call allocate 2";
      else if constexpr (std::is_same_v<T, L3::CallTupleError>)
        out << "call tuple-error 3";
      else if constexpr (std::is_same_v<T, L3::Increment>)
        out << w_to_str(ins.reg) << "++";
      else if constexpr (std::is_same_v<T, L3::Decrement>)
        out << w_to_str(ins.reg) << "--";
      else if constexpr (std::is_same_v<T, L3::CallTensorError>)
        out << "call tensor-error " << ins.F;
      else if constexpr (std::is_same_v<T, L3::Compare>)
        out << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, L3::Cjump>)
        out << "cjump " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right) << " " << ins.target.name;
      else if constexpr (std::is_same_v<T, L3::Goto>)
        out << "goto " << ins.target.name;
      else if constexpr (std::is_same_v<T, L3::MemRead>)
        out << w_to_str(ins.dest) << " <- mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, L3::MemWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::MemAopRead>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.operation) << " mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, L3::MemAopWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " " << aop_to_str(ins.operation) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, L3::StackArg>)
        out << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      else if constexpr (std::is_same_v<T, L3::Call>)
        out << "call " << std::visit([&](auto &v) -> std::string {
          using U = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<U, L3::r>)
            return r_to_str(v);
          else if (std::is_same_v<U, L3::var>)
            return add_perc(v.name);
          else
            return v.name;
        },
                                     ins.callee)
            << " " << ins.num_args.value;
      else if constexpr (std::is_same_v<T, L3::LoadAddr>)
        out << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale; },
               instr);
    out << "\n";
  }
  out << ")\n";
};
void print_source_program(pIR program) {
  for (auto &fn : program.functions) {
    print_functionIR(fn);
  };
};
} // namespace IR