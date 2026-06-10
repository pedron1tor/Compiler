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

#include <LB.h>
#include <parser.h>

#include "parsing/instructions.h"
#include "parsing/primitives.h"
#include "parsing/registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace LB {

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
                                        parse_scope> {
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
    LB::fLB newF;
    newF.name = in.string();
    newF.f_type = acc.parsed_types.back();
    acc.parsed_types.pop_back();
    acc.program.functions.push_back(newF);
  }
};
template <>
struct action<parse_function_rule> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.kont.clear();
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

LB::pLB parse_file(char *filename) {
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
void print_scopeLB(LB::scope &scope, std::ostream &out);

void print_instructionLB(LB::i &instr, std::ostream &out) {
  std::visit([&](auto &ins) {
    using T = std::decay_t<decltype(ins)>;
    if constexpr (std::is_same_v<T, LA::Assign>)
      out << ins.dest << " <- " << t_to_str(ins.src);
    else if constexpr (std::is_same_v<T, LA::BinOp>)
      out << ins.dest << " <- " << t_to_str(ins.left) << " " << op_to_str(ins.operation) << " " << t_to_str(ins.right);
    else if constexpr (std::is_same_v<T, LA::CallArbitrary>)
      out << ins.callee << call_args_to_str(ins.args);
    else if constexpr (std::is_same_v<T, LA::CallAssignArbitrary>)
      out << ins.dest << " <- " << ins.callexpr.callee << call_args_to_str(ins.callexpr.args);
    else if constexpr (std::is_same_v<T, LA::ArrayRead>)
      out << ins.dest << " <- " << ins.src << indices_to_str(ins.indices);
    else if constexpr (std::is_same_v<T, LA::ArrayWrite>)
      out << ins.dest << indices_to_str(ins.indices) << " <- " << t_to_str(ins.src);
    else if constexpr (std::is_same_v<T, LA::Length>) {
      out << ins.dest << " <- length " << ins.src;
      if (ins.dim)
        out << " " << t_to_str(*ins.dim);
    } else if constexpr (std::is_same_v<T, LA::NewArray>)
      out << ins.dest << " <- new Array(" << dims_to_str(ins.dims) << ")";
    else if constexpr (std::is_same_v<T, LA::NewTuple>)
      out << ins.dest << " <- new Tuple(" << t_to_str(ins.size) << ")";
    else if constexpr (std::is_same_v<T, LB::TypeVars>) {
      out << type_to_str(ins.var_type);
      bool first = true;
      for (auto var : ins.vars_declared) {
        if (!first) {
          out << ", ";
        }
        out << var;
      }

    } else if constexpr (std::is_same_v<T, LB::Continue>) {
      out << "continue";
    } else if constexpr (std::is_same_v<T, LB::Break>) {
      out << "break";
    } else if constexpr (std::is_same_v<T, LB::ReturnTOpt>) {
      out << "return";
      if (ins.return_thing) {
        out << " " << t_to_str(ins.return_thing.value());
      }
    } else if constexpr (std::is_same_v<T, L3::Goto>)
      std::cout << "goto " << ins.target.name;
    else if constexpr (std::is_same_v<T, LA::BrCond>)
      out << "br " << t_to_str(ins.cond) << " " << ins.true_label << " " << ins.false_label;
    else if constexpr (std::is_same_v<T, L3::LabelInst>)
      out << ins.lbl.name;
    else if constexpr (std::is_same_v<T, L3::brlabel>)
      out << "br " << ins.lbl.name;
    else if constexpr (std::is_same_v<T, std::shared_ptr<LB::scope>>)
      print_scopeLB(*ins, out);
  },
             instr);
};
void print_scopeLB(LB::scope &scope, std::ostream &out) {
  out << " {\n";
  for (auto &instr : scope.instructions) {
    out << "\t";
    print_instructionLB(instr, out);

    out << "\n";
  };

  out << "}\n";
};

void print_functionLB(LB::fLB &fn, bool generator, std::ofstream *outputFile) {
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
  out << ")";
  print_scopeLB(*fn.lscope, out);
};

void print_functionLA(LA::fLA &fn, bool generator, std::ofstream *outputFile) {
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
      if constexpr (std::is_same_v<T, LA::Assign>)
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

void print_source_program(LB::pLB program) {
  for (auto &fn : program.functions) {
    print_functionLB(fn, false, nullptr);
  };
};
} // namespace LB