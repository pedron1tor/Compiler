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

#include <LA.h>
#include <parser.h>

#include "parsing/instructions.h"
#include "parsing/primitives.h"
#include "parsing/registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace LA {

/*
 * Grammar rules definitions from now on.
 */

struct parse_function_name : pegtl::seq<parse_name> {
};
// pars ::= type var | type var (, type var)* |
struct parse_pars_type : parse_type {
};
struct parse_pars_var : parse_variable {
};
struct parse_pars : pegtl::seq<parse_pars_type, parse_spaces, parse_pars_var> {
};
struct parse_vars : pegtl::seq<
                        pegtl::one<'('>, parse_seps,
                        pegtl::opt<pegtl::seq<
                            parse_pars,
                            pegtl::star<pegtl::seq<parse_seps, parse_comma, parse_seps, parse_pars>>>>,
                        parse_seps, pegtl::one<')'>> {
};
struct parse_function_rule : pegtl::seq<parse_seps,
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
    LA::fLA newF;
    newF.name = in.string();
    newF.f_type = acc.parsed_types.back();
    acc.parsed_types.pop_back();
    acc.program.functions.push_back(newF);
  }
};

template <>
struct action<parse_pars_var> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};

template <>
struct action<parse_pars> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto var_name = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto var_type = acc.parsed_types.back();
    acc.parsed_types.pop_back();
    acc.program.functions.back().parameters.input_variables.push_back(LA::TypeVar{var_type, var_name});
  }
};

pLA parse_file(char *filename) {
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
void print_functionLA(fLA &fn, bool generator, std::ofstream *outputFile) {
  std::ostream *outp = (generator && outputFile) ? static_cast<std::ostream *>(outputFile) : &std::cout;
  std::ostream &out = *outp;
  out << type_to_str(fn.f_type) << " " << fn.name << " (";
  bool first = true;
  for (auto &tv : fn.parameters.input_variables) {
    if (!first)
      out << ", ";
    first = false;
    out << type_to_str(tv.var_type) << tv.var_declared;
  }
  out << ") {\n";
  for (auto &instr : fn.instructions) {
    out << "\t";
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, Assign>)
        out << ins.dest << " <- " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, LA::BinOp>)
        out << ins.dest << " <- " << t_to_str(ins.left) << " " << op_to_str(ins.operation) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, LA::CallArbitrary>)
        out << ins.callee << call_args_to_str(ins.args);
      else if constexpr (std::is_same_v<T, LA::CallAssignArbitrary>)
        out <<  ins.dest << " <- " << ins.callexpr.callee << call_args_to_str(ins.callexpr.args);
      else if constexpr (std::is_same_v<T, LA::ArrayRead>)
        out << ins.dest << " <- " << ins.src << indices_to_str(ins.indices);
      else if constexpr (std::is_same_v<T, LA::ArrayWrite>)
        out <<  ins.dest << indices_to_str(ins.indices) << " <- " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, LA::Length>) {
        out << ins.dest << " <- length " << ins.src;
        if (ins.dim) out << " " << t_to_str(*ins.dim);
      }
      else if constexpr (std::is_same_v<T, LA::NewArray>)
        out <<  ins.dest << " <- new Array(" << dims_to_str(ins.dims) << ")";
      else if constexpr (std::is_same_v<T, LA::NewTuple>)
        out <<  ins.dest << " <- new Tuple(" << t_to_str(ins.size) << ")";
      else if constexpr (std::is_same_v<T, LA::TypeVar>)
        out << type_to_str(ins.var_type) <<  ins.var_declared;
      else if constexpr (std::is_same_v<T, L3::Return>)
        out
            << "return";
      else if constexpr (std::is_same_v<T, LA::ReturnT>)
        out<< "return " << t_to_str(ins.return_thing);

      else if constexpr (std::is_same_v<T, LA::BrCond>)
        out << "br " << t_to_str(ins.cond) << " " << ins.true_label << " " << ins.false_label;
      else if constexpr (std::is_same_v<T, L3::LabelInst>)
        out << ins.lbl.name;
      else if constexpr (std::is_same_v<T, L3::brlabel>)
        out << "br " << ins.lbl.name; },
               instr);
    out << "\n";
  };

  out << "}\n";
};

void print_functionIR(IR::fIR &fn, bool generator, std::ofstream *outputFile) {
  std::ostream *outp = (generator && outputFile) ? static_cast<std::ostream *>(outputFile) : &std::cout;
  std::ostream &out = *outp;
  out << "define " << type_to_str(fn.f_type) << fn.name.name << " (";
  bool first = true;
  for (auto &tv : fn.parameters.input_variables) {
    if (!first)
      out << ", ";
    first = false;
    out << type_to_str(tv.var_type) << add_perc(tv.var_declared.name);
  }
  out << ") {\n";
  for (auto &block : fn.blocks) {
    out << "\t";
    out << block.lbl.name << "\n";
    for (auto &instr : block.instructions) {
      out << "\t";
      std::visit([&](auto &ins) {
          using T = std::decay_t<decltype(ins)>;
          if constexpr (std::is_same_v<T, L3::Assign>)
            out << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
          else if constexpr (std::is_same_v<T, IR::BinOp>)
            out << add_perc(ins.dest.name) << " <- " << L3t_to_str(ins.left) << " " << op_to_str(ins.operation) << " " << L3t_to_str(ins.right);
          else if constexpr (std::is_same_v<T, L3::CallL3>)
            out << "call " << callee_to_str(ins.callee) << L3call_args_to_str(ins.args);
          else if constexpr (std::is_same_v<T, L3::CallAssignL3>)
            out << add_perc(ins.dest.name) << " <- call " << callee_to_str(ins.callexpr.callee) << L3call_args_to_str(ins.callexpr.args);
          else if constexpr (std::is_same_v<T, L3::CallPrint>)
            out << "call print()";
          else if constexpr (std::is_same_v<T, L3::CallInput>)
            out << "call input()";
          else if constexpr (std::is_same_v<T, L3::CallTupleError>)
            out << "call tuple-error()";
          else if constexpr (std::is_same_v<T, L3::CallTensorError>)
            out << "call tensor-error()";
          else if constexpr (std::is_same_v<T, IR::ArrayRead>)
            out << add_perc(ins.dest.name) << " <- " << add_perc(ins.src.name) << L3indices_to_str(ins.indices);
          else if constexpr (std::is_same_v<T, IR::ArrayWrite>)
            out << add_perc(ins.dest.name) << L3indices_to_str(ins.indices) <<  " <- " << s_to_str(ins.src) ;
          else if constexpr (std::is_same_v<T, IR::Length>) {
            out << add_perc(ins.dest.name) << " <- length " << add_perc(ins.src.name);
            if (ins.dim.has_value()) out << " " << L3t_to_str(*ins.dim);
          }
          else if constexpr (std::is_same_v<T, IR::NewArray>)
            out << add_perc(ins.dest.name) << " <- new Array(" << L3dims_to_str(ins.dims) << ")";
          else if constexpr (std::is_same_v<T, IR::NewTuple>)
            out << add_perc(ins.dest.name) << " <- new Tuple(" << L3t_to_str(ins.size)<< ")";
          else if constexpr (std::is_same_v<T, IR::TypeVar>)
            out << type_to_str(ins.var_type) << add_perc(ins.var_declared.name); },
                 instr);
      out << "\n";
    }
    out << "\t";
    std::visit([&](auto &te_inst) {
        using T = std::decay_t<decltype(te_inst)>;
        if constexpr (std::is_same_v<T, L3::Return>)
          out << "return";
        else if constexpr (std::is_same_v<T, L3::ReturnT>)
          out << "return " << L3t_to_str(te_inst.return_thing);
        else if constexpr (std::is_same_v<T, L3::Goto>)
          out << "br " << te_inst.target.name;
        else if constexpr (std::is_same_v<T, IR::BrCond>)
          out << "br " << L3t_to_str(te_inst.cond) << " " << te_inst.true_label.name << " " << te_inst.false_label.name; },
               block.terminal);
    out << "\n";
  }
  out << "}\n";
};

void print_source_program(pLA program) {
  for (auto &fn : program.functions) {
    print_functionLA(fn, false, nullptr);
  };
};
} // namespace LA