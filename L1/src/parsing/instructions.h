#pragma once

#include <L1.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include "primitives.h"
#include "registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L1 {
struct parse_return : pegtl::seq<pegtl::string<'r', 'e', 't', 'u', 'r', 'n'>, parse_nochar> {};
struct parse_call : pegtl::seq<pegtl::string<'c', 'a', 'l', 'l'>, parse_nochar> {};
struct parse_print : pegtl::seq<pegtl::string<'p', 'r', 'i', 'n', 't'>, parse_nochar> {};
struct parse_input : pegtl::seq<pegtl::string<'i', 'n', 'p', 'u', 't'>, parse_nochar> {};
struct parse_allocate : pegtl::seq<pegtl::string<'a', 'l', 'l', 'o', 'c', 'a', 't', 'e'>, parse_nochar> {};
struct parse_tuple_error : pegtl::seq<pegtl::string<'t', 'u', 'p', 'l', 'e', '-', 'e', 'r', 'r', 'o', 'r'>, parse_nochar> {};
struct parse_tensor_error : pegtl::seq<pegtl::string<'t', 'e', 'n', 's', 'o', 'r', '-', 'e', 'r', 'r', 'o', 'r'>, parse_nochar> {};
struct parse_call_print : pegtl::seq<parse_call, parse_spaces, parse_print, parse_spaces, parse_number> {};
struct parse_call_input : pegtl::seq<parse_call, parse_spaces, parse_input, parse_spaces, parse_number> {};
struct parse_call_allocate : pegtl::seq<parse_call, parse_spaces, parse_allocate, parse_spaces, parse_number> {};
struct parse_call_tuple_error : pegtl::seq<parse_call, parse_spaces, parse_tuple_error, parse_spaces, parse_number> {};
struct parse_tensor_error_F : parse_number {};
struct parse_call_tensor_error : pegtl::seq<parse_call, parse_spaces, parse_tensor_error, parse_spaces, parse_tensor_error_F> {};
// s value variants
struct parse_s_num_val   : parse_number {};
struct parse_s_label_val : parse_label {};
struct parse_s_fname_val : pegtl::seq<pegtl::one<'@'>, parse_name> {};
struct parse_s_val : pegtl::sor<parse_s_label_val, parse_s_fname_val, parse_register, parse_s_num_val> {};
// w <- s
struct parse_assignment : pegtl::seq<
                              parse_register,
                              parse_spaces,
                              parse_arrow,
                              parse_spaces,
                              parse_s_val> {};
struct parse_num_t : parse_number {};
struct parse_x_t : parse_register {};
// w <- mem x M
// mem x M <- s
// w aop t
struct parse_aop_expression : pegtl::seq<parse_register, parse_spaces, parse_aop, parse_spaces, pegtl::sor<parse_x_t, parse_num_t>> {};
// w sop sx
// w sop N
// mem x M += t
// mem x M -= t
// w += mem x M
// w -= mem x M
// w <- t cmp t
// cjump t cmp t label
struct parse_label_inst : parse_label {};
// goto label

struct parse_instructions : pegtl::seq<parse_seps,
                                       pegtl::plus<pegtl::seq<parse_spaces, pegtl::sor<parse_return, parse_call_print, parse_call_input, parse_call_allocate, parse_call_tuple_error, parse_call_tensor_error, parse_label_inst, parse_aop_expression, parse_assignment>, parse_spaces, parse_seps>>> {};

/*
templates
*/
template <typename Rule, typename Inst>
struct action_push {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto &currentF = program.functions.back();
    currentF.instructions.push_back(i{Inst{}});
  }
};
template <>
struct action<parse_return> : action_push<parse_return, Return> {};
template <>
struct action<parse_call_print> : action_push<parse_call_print, CallPrint> {};
template <>
struct action<parse_call_input> : action_push<parse_call_input, CallInput> {};
template <>
struct action<parse_call_allocate> : action_push<parse_call_allocate, CallAllocate> {};
template <>
struct action<parse_call_tuple_error> : action_push<parse_call_tuple_error, CallTupleError> {};
template <>
struct action<parse_tensor_error_F> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items_t.push_back(t{N{std::stoll(in.string())}});
  }
};
template <>
struct action<parse_call_tensor_error> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto F = std::get<N>(parsed_items_t.back()).value;
    parsed_items_t.pop_back();
    program.functions.back().instructions.push_back(i{CallTensorError{F}});
  }
};

template <>
struct action<parse_label_inst> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    program.functions.back().instructions.push_back(i{LabelInst{label{in.string()}}});
  }
};
template <>
struct action<parse_num_t> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items_t.push_back(t{N{std::stoll(in.string())}});
  }
};
template <>
struct action<parse_x_t> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto reg = parsed_items.back();
    parsed_items.pop_back();
    parsed_items_t.push_back(t{std::get<r>(reg)});
  }
};
template <>
struct action<parse_aop> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    if (in.string() == "+=") {
      parsed_ops.push_back(aop::add);
    } else if (in.string() == "-=") {
      parsed_ops.push_back(aop::sub);
    } else if (in.string() == "*=") {
      parsed_ops.push_back(aop::mul);
    } else if (in.string() == "&=") {
      parsed_ops.push_back(aop::and_);
    }
  }
};
template <>
struct action<parse_s_num_val> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{N{std::stoll(in.string())}});
  }
};
template <>
struct action<parse_s_label_val> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{label{in.string()}});
  }
};
template <>
struct action<parse_s_fname_val> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{l{in.string()}});
  }
};
template <>
struct action<parse_assignment> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto src = parsed_items.back();
    parsed_items.pop_back();
    auto dst = parsed_items.back();
    parsed_items.pop_back();
    auto newI = Assign{std::get<r>(dst), src};
    program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_aop_expression> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    try {
   auto src_item = parsed_items_t.back();
    parsed_items_t.pop_back();
    t src = std::holds_alternative<r>(src_item)
                ? t{std::get<r>(src_item)}
                : t{std::get<N>(src_item)};
    auto aop = parsed_ops.back();
    parsed_ops.pop_back();
    auto dst = parsed_items.back();
    parsed_items.pop_back();
    auto newI = AopInst{std::get<r>(dst), aop, src};
    program.functions.back().instructions.push_back(i{newI});
} catch (std::exception& e) {
    std::cerr << "action crashed: " << e.what() << "\n";
    throw;
}
    
  }
};
} // namespace L1