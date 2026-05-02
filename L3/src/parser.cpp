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

#include <L3.h>
#include <parser.h>

#include "parsing/instructions.h"
#include "parsing/primitives.h"
#include "parsing/registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L3
{

  /*
   * Grammar rules definitions from now on.
   */

  struct parse_function_name : pegtl::seq<pegtl::one<'@'>, parse_name>
  {
  };
  struct parse_vars_var : parse_variable
  {
  };
  struct parse_vars : pegtl::seq<
                          pegtl::one<'('>,
                          parse_seps,
                          pegtl::opt<pegtl::seq<
                              parse_vars_var,
                              pegtl::star<pegtl::seq<parse_seps, parse_comma, parse_seps, parse_vars_var>>>>,
                          parse_seps,
                          pegtl::one<')'>>
  {
  };
  struct parse_function_rule : pegtl::seq<parse_seps,
                                          parse_define,
                                          parse_seps,
                                          parse_function_name,
                                          parse_seps,
                                          parse_vars,
                                          parse_seps,
                                          pegtl::one<'{'>,
                                          parse_instructions,
                                          parse_seps,
                                          pegtl::one<'}'>>
  {
  };
  struct parse_spill_arg_1 : parse_variable
  {
  };
  struct parse_spill_arg_2 : parse_variable
  {
  };
  struct parse_function_rule_spill : pegtl::seq<parse_function_rule, parse_seps, parse_spill_arg_1, parse_seps, parse_spill_arg_2>
  {
  };
  struct parse_functions_rule : pegtl::seq<
                                    pegtl::plus<parse_function_rule>>
  {
  };
  struct parse_entry_point_rule : pegtl::seq<
                                      parse_seps,
                                      parse_functions_rule,
                                      parse_seps>
  {
  };

  struct grammar : pegtl::seq<
                       parse_entry_point_rule, pegtl::eof>
  {
  };

  /*
   * templates
   */
  template <>
  struct action<parse_function_name>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {

      fl3 newF;
      newF.name.name = in.string();
      acc.program.functions.push_back(newF);
    }
  };

  template <>
  struct action<parse_vars_var>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.program.functions.back().vars.push_back(var{remove_perc(in.string())});
    }
  };

  template <>
  struct action<parse_spill_arg_1>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_spill_args.push_back(var{remove_perc(in.string())});
    }
  };
  template <>
  struct action<parse_spill_arg_2>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_spill_args.push_back(var{remove_perc(in.string())});
    }
  };
  pl3 parse_file(char *filename)
  {
    /*
     * Check the grammar for some possible issues.
     */
    if (pegtl::analyze<grammar>() != 0)
    {
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

  std::string
  r_to_str(r reg)
  {
    switch (reg)
    {
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
    __builtin_unreachable();
  };
  std::string t_to_str(t val)
  {
    return std::visit([](auto &v) -> std::string
                      {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return std::to_string(v.value); },
                      val);
  }
  std::string s_to_str(s val)
  {
    return std::visit([](auto &v) -> std::string
                      {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, N>)
      return std::to_string(v.value);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return v.name; },
                      val);
  }
  std::string w_to_str(w val)
  {
    return std::visit([](auto &v) -> std::string
                      {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return v.name; },
                      val);
  }
  std::string aop_to_str(aop op)
  {
    switch (op)
    {
    case aop::add:
      return "+";
    case aop::sub:
      return "-";
    case aop::mul:
      return "*";
    case aop::and_:
      return "&";
    case aop::lshift:
      return "<<";
    case aop::rshift:
      return ">>";
    }
    __builtin_unreachable();
  }
  std::string sop_to_str(sop op)
  {
    switch (op)
    {
    case sop::lshift:
      return "<<=";
    case sop::rshift:
      return ">>=";
    }
    __builtin_unreachable();
  }
  std::string cmp_to_str(cmp op)
  {
    switch (op)
    {
    case cmp::lt:
      return "<";
    case cmp::gt:
      return ">";
    case cmp::le:
      return "<=";
    case cmp::eq:
      return "=";
    case cmp::gte:
      return ">=";
    }
    __builtin_unreachable();
  }

  std::string callee_to_str(callee_type &callee)
  {
    return std::visit([](auto &c) -> std::string
                      {
      using C = std::decay_t<decltype(c)>;
      if constexpr (std::is_same_v<C, CallPrint>) return "print";
      else if constexpr (std::is_same_v<C, CallInput>) return "input";
      else if constexpr (std::is_same_v<C, CallAllocate>) return "allocate";
      else if constexpr (std::is_same_v<C, CallTupleError>) return "tuple-error";
      else if constexpr (std::is_same_v<C, CallTensorError>) return "tensor-error";
      else return std::visit([](auto &v) -> std::string {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, r>) return r_to_str(v);
        else if constexpr (std::is_same_v<V, var>) return add_perc(v.name);
        else return v.name;
      }, c); }, callee);
  }
  std::string call_args_to_str(std::vector<t> &args)
  {
    std::string result = "(";
    bool first = true;
    for (auto &arg : args)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        result += ", ";
      }

      result += t_to_str(arg);
    }
    return result + ")";
  }

  void print_functionl3(fl3 &fn)
  {
    std::cout << "define " << fn.name.name;
    // print variables
    std::cout << " (";
    bool first = true;
    for (var &variable : fn.vars)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        std::cout << ", ";
      }
      std::cout << add_perc(variable.name);
    }
    std::cout << ")";
    std::cout << "{\n";
    for (auto &instr : fn.instructions)
    {
      std::cout << "\t";
      std::visit([&](auto &ins)
                 {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, Return>)
        std::cout << "return";
      else if constexpr (std::is_same_v<T, ReturnT>)
        std::cout << "return " << t_to_str(ins.return_thing); 
      else if constexpr (std::is_same_v<T, Assign>)
        std::cout << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, AopInst>)
        std::cout << w_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, LabelInst>)
        std::cout << ins.lbl.name;
     
    
      else if constexpr (std::is_same_v<T, Compare>)
        std::cout << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, StackArg>)
        std::cout << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      else if constexpr (std::is_same_v<T, LoadAddr>)
        std::cout << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale;
      else if constexpr (std::is_same_v<T, AopL3>)
        std::cout << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, StoreL3>)
        std::cout << "store " << add_perc(ins.dest.name) << " <- " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, LoadL3>)
        std::cout << add_perc(ins.dest.name) << " <- load " << add_perc(ins.src.name);
      else if constexpr (std::is_same_v<T, brlabel>)
        std::cout << "br " << ins.label.name;
      else if constexpr (std::is_same_v<T, brtlabel>)
        std::cout << "br " << t_to_str(ins.cond) << " " << ins.label.name;
      else if constexpr (std::is_same_v<T, CallL3>)
        std::cout << "call " << callee_to_str(ins.callee) << call_args_to_str(ins.args);
      else if constexpr (std::is_same_v<T, CallAssignL3>)
        std::cout << add_perc(ins.dest.name) << " <- call " << callee_to_str(ins.callexpr.callee) << call_args_to_str(ins.callexpr.args); },
                 instr);
      std::cout << "\n";
    }
    std::cout << "}\n";
  };
  void print_functionl2(fl2 &fn, bool generator, std::ofstream *outputFile)
  {
    std::ostream *outp = (generator && outputFile) ? static_cast<std::ostream *>(outputFile) : &std::cout;
    std::ostream &out = *outp;
    out << "(" << fn.name.name << "\n";
    out << "\t" << fn.num_args << "\n";
    for (auto &instr : fn.instructions)
    {
      out << "\t";
      std::visit([&](auto &ins)
                 {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, Return>)
        out << "return";
      else if constexpr (std::is_same_v<T, Assign>)
        out << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, AopInst>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, SopReg>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << t_to_str(ins.shift);
      else if constexpr (std::is_same_v<T, SopNum>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << ins.shift.value;
      else if constexpr (std::is_same_v<T, LabelInst>)
        out << ins.lbl.name;
      else if constexpr (std::is_same_v<T, CallPrint>)
        out << "call print 1";
      else if constexpr (std::is_same_v<T, CallInput>)
        out << "call input 0";
      else if constexpr (std::is_same_v<T, CallAllocate>)
        out << "call allocate 2";
      else if constexpr (std::is_same_v<T, CallTupleError>)
        out << "call tuple-error 3";
      else if constexpr (std::is_same_v<T, Increment>)
        out << w_to_str(ins.reg) << "++";
      else if constexpr (std::is_same_v<T, Decrement>)
        out << w_to_str(ins.reg) << "--";
      else if constexpr (std::is_same_v<T, CallTensorError>)
        out << "call tensor-error " << ins.F;
      else if constexpr (std::is_same_v<T, Compare>)
        out << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, Cjump>)
        out << "cjump " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right) << " " << ins.target.name;
      else if constexpr (std::is_same_v<T, Goto>)
        out << "goto " << ins.target.name;
      else if constexpr (std::is_same_v<T, MemRead>)
        out << w_to_str(ins.dest) << " <- mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, MemWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, MemAopRead>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.operation) << " mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, MemAopWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " " << aop_to_str(ins.operation) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, StackArg>)
        out << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      else if constexpr (std::is_same_v<T, Call>)
        out << "call " << std::visit([&](auto &v) -> std::string {
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
        out << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale; },
                 instr);
      out << "\n";
    }
    out << ")\n";
  };
  void print_functionl1(fl1 &fn, bool generator, std::ofstream *outputFile)
  {
    std::ostream *outp = (generator && outputFile) ? static_cast<std::ostream *>(outputFile) : &std::cout;
    std::ostream &out = *outp;
    out << "(" << fn.name.name << "\n";
    out << "    " << fn.num_args << " " << fn.num_locals << "\n";
    for (auto &instr : fn.instructions)
    {
      out << "\t";
      std::visit([&](auto &ins)
                 {
      using T = std::decay_t<decltype(ins)>;
      if constexpr (std::is_same_v<T, Return>)
        out << "return";
      else if constexpr (std::is_same_v<T, Assign>)
        out << w_to_str(ins.dest) << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, AopInst>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.op) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, SopReg>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << t_to_str(ins.shift);
      else if constexpr (std::is_same_v<T, SopNum>)
        out << w_to_str(ins.dest) << " " << sop_to_str(ins.op) << " " << ins.shift.value;
      else if constexpr (std::is_same_v<T, LabelInst>)
        out << ins.lbl.name;
      else if constexpr (std::is_same_v<T, CallPrint>)
        out << "call print 1";
      else if constexpr (std::is_same_v<T, CallInput>)
        out << "call input 0";
      else if constexpr (std::is_same_v<T, CallAllocate>)
        out << "call allocate 2";
      else if constexpr (std::is_same_v<T, CallTupleError>)
        out << "call tuple-error 3";
      else if constexpr (std::is_same_v<T, Increment>)
        out << w_to_str(ins.reg) << "++";
      else if constexpr (std::is_same_v<T, Decrement>)
        out << w_to_str(ins.reg) << "--";
      else if constexpr (std::is_same_v<T, CallTensorError>)
        out << "call tensor-error " << ins.F;
      else if constexpr (std::is_same_v<T, Compare>)
        out << w_to_str(ins.dest) << " <- " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right);
      else if constexpr (std::is_same_v<T, Cjump>)
        out << "cjump " << t_to_str(ins.left) << " " << cmp_to_str(ins.op) << " " << t_to_str(ins.right) << " " << ins.target.name;
      else if constexpr (std::is_same_v<T, Goto>)
        out << "goto " << ins.target.name;
      else if constexpr (std::is_same_v<T, MemRead>)
        out << w_to_str(ins.dest) << " <- mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, MemWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " <- " << s_to_str(ins.src);
      else if constexpr (std::is_same_v<T, MemAopRead>)
        out << w_to_str(ins.dest) << " " << aop_to_str(ins.operation) << " mem " << w_to_str(ins.base) << " " << ins.offset;
      else if constexpr (std::is_same_v<T, MemAopWrite>)
        out << "mem " << w_to_str(ins.base) << " " << ins.offset << " " << aop_to_str(ins.operation) << " " << t_to_str(ins.src);
      else if constexpr (std::is_same_v<T, StackArg>)
        out << w_to_str(ins.dest) << " <- " << "stack-arg " << std::to_string(ins.num.value);
      else if constexpr (std::is_same_v<T, Call>)
        out << "call " << std::visit([&](auto &v) -> std::string {
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
        out << w_to_str(ins.dest) << " @ " << w_to_str(ins.base) << " " << w_to_str(ins.index) << " " << ins.scale; },
                 instr);
      out << "\n";
    }
    out << ")\n";
  };
  void print_source_program(pl3 program)
  {
    for (auto &fn : program.functions)
    {
      print_functionl3(fn);
    };
  };
} // namespace L3