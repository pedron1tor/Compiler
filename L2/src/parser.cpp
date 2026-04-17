#include <algorithm>
#include <assert.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
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

#include <L2.h>
#include <parser.h>

#include "parsing/instructions.h"
#include "parsing/primitives.h"
#include "parsing/registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {

/*
 * Grammar rules definitions from now on.
 */

struct parse_function_name : pegtl::seq<pegtl::one<'@'>, parse_name> {};

struct parse_argument_number : parse_number {};

struct parse_function_rule : pegtl::seq<parse_seps,
                                        pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                        parse_seps,
                                        parse_function_name,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, parse_argument_number>,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, parse_instructions>,
                                        parse_seps,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, pegtl::one<')'>>,
                                        parse_seps> {};
struct parse_functions_rule : pegtl::seq<
                                  pegtl::plus<parse_function_rule>> {};
struct parse_entry_point_rule : pegtl::seq<
                                    parse_seps,
                                    pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                    parse_seps,
                                    parse_function_name,
                                    parse_seps,
                                    parse_functions_rule,
                                    pegtl::seq<parse_spaces, pegtl::one<')'>>,
                                    parse_seps> {};

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
    if (acc.program.entry_point.name.empty()) {
      acc.program.entry_point.name = in.string();
    } else {
      f newF;
      newF.name.name = in.string();
      acc.program.functions.push_back(newF);
    }
  }
};
template <>
struct action<parse_argument_number> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto &currentF = acc.program.functions.back();
    currentF.num_args = std::stoll(in.string());
  }
};

p parse_file(char *filename) {
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

f parse_function(char *filename) {
  if (pegtl::analyze<parse_function_rule>() != 0) {
    std::cerr << "There are problems with the function grammar" << std::endl;
    exit(1);
  }
  /*
   * Parse.
   */
  file_input<> fileInput(filename);
  // run to debug program...
  // pegtl::standard_trace<parse_function_rule>(fileInput);
  GlobalAccumulator state;
  state.program.entry_point.name = '_';
  parse<parse_function_rule, action>(fileInput, state);
  return state.program.functions[0];
}

std::string
r_to_str(r reg) {
  switch (reg) {
  case r::rax:
    return "rax";
  case r::rbx:
    return "rbx";
  case r::rbp:
    return "rbp";
  case r::r10:
    return "r10";
  case r::r11:
    return "r11";
  case r::r12:
    return "r12";
  case r::r13:
    return "r13";
  case r::r14:
    return "r14";
  case r::r15:
    return "r15";
  case r::rdi:
    return "rdi";
  case r::rsi:
    return "rsi";
  case r::rdx:
    return "rdx";
  case r::r8:
    return "r8";
  case r::r9:
    return "r9";
  case r::rcx:
    return "rcx";
  case r::rsp:
    return "rsp";
  }
};
std::string t_to_str(t val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return std::to_string(v.value);
  },
                    val);
}
std::string s_to_str(s val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, N>)
      return std::to_string(v.value);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return v.name;
  },
                    val);
}
std::string w_to_str(w val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return v.name;
  },
                    val);
}
std::string aop_to_str(aop op) {
  switch (op) {
  case aop::add:
    return "+=";
  case aop::sub:
    return "-=";
  case aop::mul:
    return "*=";
  case aop::and_:
    return "&=";
  }
}
std::string sop_to_str(sop op) {
  switch (op) {
  case sop::lshift:
    return "<<=";
  case sop::rshift:
    return ">>=";
  }
}
std::string cmp_to_str(cmp op) {
  switch (op) {
  case cmp::lt:
    return "<";
  case cmp::le:
    return "<=";
  case cmp::eq:
    return "=";
  }
}

void print_function(f &fn) {
  std::cout << "(" << fn.name.name << "\n";
  std::cout << "\t" << fn.num_args << "\n";
  for (auto &instr : fn.instructions) {
    std::cout << "\t";
    std::visit([&](auto &ins) {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, Return>)
        std::cout << "return";
      else if constexpr (std::is_same_v<T, Assign>)
        std::cout << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, AopInst>)
        std::cout << w_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, SopReg>)
        std::cout << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << t_to_str(ins.shift);
      else if constexpr (std::is_same_v<T, SopNum>)
        std::cout << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << ins.shift.value;
      else if constexpr (std::is_same_v<T, LabelInst>)
        std::cout << ins.lbl.name;
      else if constexpr (std::is_same_v<T, CallPrint>)
        std::cout << "call print 1";
      else if constexpr (std::is_same_v<T, CallInput>)
        std::cout << "call input 0";
      else if constexpr (std::is_same_v<T, CallAllocate>)
        std::cout << "call allocate 2";
      else if constexpr (std::is_same_v<T, CallTupleError>)
        std::cout << "call tuple-error 3";
      else if constexpr (std::is_same_v<T, Increment>)
        std::cout << w_to_str(ins.reg) << "++";
      else if constexpr (std::is_same_v<T, Decrement>)
        std::cout << w_to_str(ins.reg) << "--";
      else if constexpr (std::is_same_v<T, CallTensorError>)
        std::cout << "call tensor-error " << ins.F;
      else if constexpr (std::is_same_v<T, Compare>)
        std::cout << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, Cjump>)
        std::cout << "cjump " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right) << " " << ins.target.name;
      else if constexpr (std::is_same_v<T, Goto>)
        std::cout << "goto " << ins.target.name;
      else if constexpr (std::is_same_v<T, MemRead>)
        std::cout << w_to_str(ins.dest) << " <- mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, MemWrite>)
        std::cout << "mem " << w_to_str(ins.base) << " " << ins.offset << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, MemAopRead>)
        std::cout << w_to_str(ins.dest) << " " << aop_to_str(ins.operation) << " mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, MemAopWrite>)
        std::cout << "mem " << w_to_str(ins.base) << " " << ins.offset << " " << aop_to_str(ins.operation) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, StackArg>)
        std::cout << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      else if constexpr (std::is_same_v<T, Call>)
        std::cout << "call " << std::visit([&](auto &v) -> std::string {
          using U = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<U, r>)
            return r_to_str(v);
          else if (std::is_same_v<U, var>)
            return add_perc(v.name);
          else
            return v.name;
        },
                                           ins.callee)
                  << " " << ins.num_args.value;
      else if constexpr (std::is_same_v<T, LoadAddr>)
        std::cout << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale;
    },
               instr);
    std::cout << "\n";
  }
  std::cout << ")\n";
};
void print_source_program(p program) {
  std::cout << "(" << program.entry_point.name << "\n";
  for (auto &fn : program.functions) {
    print_function(fn);
  };
  std::cout << ")\n";
};
} // namespace L2