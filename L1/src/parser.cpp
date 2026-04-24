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

#include <L1.h>
#include <parser.h>

#include "parsing/instructions.h"
#include "parsing/primitives.h"
#include "parsing/registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L1 {

/*
 * Grammar rules definitions from now on.
 */

struct parse_function_name : pegtl::seq<pegtl::one<'@'>, parse_name> {};

struct parse_argument_number : parse_number {};
struct parse_local_number : parse_number {};

struct parse_function_rule : pegtl::seq<parse_seps,
                                        pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                        parse_seps,
                                        parse_function_name,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, parse_argument_number>,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, parse_local_number>,
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
  static void apply(const Input &in, p &program) {
    if (program.entry_point.name.empty()) {
      program.entry_point.name = in.string();
    } else {
      f newF;
      newF.name.name = in.string();
      program.functions.push_back(newF);
    }
  }
};
template <>
struct action<parse_argument_number> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto &currentF = program.functions.back();
    currentF.num_args = std::stoll(in.string());
  }
};
template <>
struct action<parse_local_number> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto &currentF = program.functions.back();
    currentF.num_locals = std::stoll(in.string());
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
  p program_ast;
  // run to debug program...
  // pegtl::standard_trace<grammar>(fileInput);
  parse<grammar, action>(fileInput, program_ast);
  return program_ast;
}
std::string r_to_str(L1::r reg) {
  switch (reg) {
  case L1::r::rax:
    return "rax";
  case L1::r::rbx:
    return "rbx";
  case L1::r::rbp:
    return "rbp";
  case L1::r::r10:
    return "r10";
  case L1::r::r11:
    return "r11";
  case L1::r::r12:
    return "r12";
  case L1::r::r13:
    return "r13";
  case L1::r::r14:
    return "r14";
  case L1::r::r15:
    return "r15";
  case L1::r::rdi:
    return "rdi";
  case L1::r::rsi:
    return "rsi";
  case L1::r::rdx:
    return "rdx";
  case L1::r::r8:
    return "r8";
  case L1::r::r9:
    return "r9";
  case L1::r::rcx:
    return "rcx";
  case L1::r::rsp:
    return "rsp";
  }
  __builtin_unreachable();
};
std::string t_to_str(L1::t val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, L1::r>)
      return r_to_str(v);
    else
      return std::to_string(v.value);
  },
                    val);
}
std::string s_to_str(L1::s val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, L1::r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, L1::N>)
      return std::to_string(v.value);
    else
      return v.name;
  },
                    val);
}
std::string aop_to_str(L1::aop op) {
  switch (op) {
  case L1::aop::add:
    return "+=";
  case L1::aop::sub:
    return "-=";
  case L1::aop::mul:
    return "*=";
  case L1::aop::and_:
    return "&=";
  }
  __builtin_unreachable();
}
std::string sop_to_str(L1::sop op) {
  switch (op) {
  case L1::sop::lshift:
    return "<<=";
  case L1::sop::rshift:
    return ">>=";
  }
  __builtin_unreachable();
}
std::string cmp_to_str(L1::cmp op) {
  switch (op) {
  case L1::cmp::lt:
    return "<";
  case L1::cmp::le:
    return "<=";
  case L1::cmp::eq:
    return "=";
  }
  __builtin_unreachable();
}
void print_source_program(p program) {
  std::cout << "(" << program.entry_point.name << "\n";
  for (auto &fn : program.functions) {
    std::cout << "  (" << fn.name.name << "\n";
    std::cout << "    " << fn.num_args << " " << fn.num_locals << "\n";
    for (auto &instr : fn.instructions) {
      std::cout << "    ";
      std::visit([&](auto &ins) {
        using T = std::decay_t<decltype(ins)>;
        if constexpr (std::is_same_v<T, L1::Return>)
          std::cout << "return";
        else if constexpr (std::is_same_v<T, L1::Assign>)
          std::cout << r_to_str(ins.dest) << " <- " << s_to_str(ins.src);
        else if constexpr (std::is_same_v<T, L1::AopInst>)
          std::cout << r_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
        else if constexpr (std::is_same_v<T, L1::SopReg>)
          std::cout << r_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << r_to_str(ins.shift);
        else if constexpr (std::is_same_v<T, L1::SopNum>)
          std::cout << r_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << ins.shift.value;
        else if constexpr (std::is_same_v<T, L1::LabelInst>)
          std::cout << ins.lbl.name;
        else if constexpr (std::is_same_v<T, L1::CallPrint>)
          std::cout << "call print 1";
        else if constexpr (std::is_same_v<T, L1::CallInput>)
          std::cout << "call input 0";
        else if constexpr (std::is_same_v<T, L1::CallAllocate>)
          std::cout << "call allocate 2";
        else if constexpr (std::is_same_v<T, L1::CallTupleError>)
          std::cout << "call tuple-error 3";
        else if constexpr (std::is_same_v<T, L1::Increment>)
          std::cout << r_to_str(ins.reg) << "++";
        else if constexpr (std::is_same_v<T, L1::Decrement>)
          std::cout << r_to_str(ins.reg) << "--";
        else if constexpr (std::is_same_v<T, L1::CallTensorError>)
          std::cout << "call tensor-error " << ins.F;
        else if constexpr (std::is_same_v<T, L1::Compare>)
          std::cout << r_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
        else if constexpr (std::is_same_v<T, L1::Cjump>)
          std::cout << "cjump " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right) << " " << ins.target.name;
        else if constexpr (std::is_same_v<T, L1::Goto>)
          std::cout << "goto " << ins.target.name;
        else if constexpr (std::is_same_v<T, L1::MemRead>)
          std::cout << r_to_str(ins.dest) << " <- mem " << r_to_str(ins.base) << " " << ins.offset;
        else if constexpr (std::is_same_v<T, L1::MemWrite>)
          std::cout << "mem " << r_to_str(ins.base) << " " << ins.offset << " <- " << s_to_str(ins.src);
        else if constexpr (std::is_same_v<T, L1::MemAopRead>)
          std::cout << r_to_str(ins.dest) << " " << aop_to_str(ins.operation) << " mem " << r_to_str(ins.base) << " " << ins.offset;
        else if constexpr (std::is_same_v<T, L1::MemAopWrite>)
          std::cout << "mem " << r_to_str(ins.base) << " " << ins.offset << " " << aop_to_str(ins.operation) << " " << t_to_str(ins.src);
        else if constexpr (std::is_same_v<T, L1::Call>)
          std::cout << "call " << std::visit([&](auto &v) -> std::string {
            using U = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<U, L1::r>)
              return r_to_str(v);
            else
              return v.name;
          },
                                             ins.callee)
                    << " " << ins.num_args.value;
        else if constexpr (std::is_same_v<T, L1::LoadAddr>)
          std::cout << r_to_str(ins.dest) << " @ " << r_to_str(ins.base) << " " << r_to_str(ins.index) << " " << ins.scale;
      },
                 instr);
      std::cout << "\n";
    }
    std::cout << "  )\n";
  }
  std::cout << ")\n";
};
} // namespace L1