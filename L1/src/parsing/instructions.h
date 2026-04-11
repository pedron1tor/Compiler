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
struct parse_mem : pegtl::seq<pegtl::string<'m', 'e', 'm'>, parse_nochar> {};
struct parse_goto : pegtl::seq<pegtl::string<'g', 'o', 't', 'o'>, parse_nochar> {};
struct parse_cjump : pegtl::seq<pegtl::string<'c', 'j', 'u', 'm', 'p'>, parse_nochar> {};
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
struct parse_increment : pegtl::seq<parse_register, pegtl::string<'+', '+'>> {};
struct parse_decrement : pegtl::seq<parse_register, pegtl::string<'-', '-'>> {};
struct parse_call_tensor_error : pegtl::seq<parse_call, parse_spaces, parse_tensor_error, parse_spaces, parse_tensor_error_F> {};
// s value variants
struct parse_s_num_val : parse_number {};
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
struct parse_M : parse_number {};
struct parse_memread_expression : pegtl::seq<parse_register,
                                             parse_spaces, parse_arrow, parse_spaces, parse_mem, parse_spaces, parse_register, parse_spaces, parse_M> {};
// mem x M <- s
struct parse_memwrite_expression : pegtl::seq<parse_mem, parse_spaces, parse_register, parse_spaces, parse_M, parse_spaces, parse_arrow, parse_spaces, parse_s_val> {};
// w aop t
struct parse_aop_expression : pegtl::seq<parse_register, parse_spaces, parse_aop, parse_spaces, pegtl::sor<parse_x_t, parse_num_t>> {};
// w sop sx
// w sop N
struct parse_sop_expression : pegtl::seq<parse_register, parse_spaces, parse_sop, parse_spaces, pegtl::sor<parse_x_t, parse_num_t>> {};
// mem x M += t
// mem x M -= t
struct parse_mem_aop_write : pegtl::seq<parse_mem, parse_spaces, parse_register, parse_spaces, parse_M, parse_spaces, parse_aop, parse_spaces,
                                        pegtl::sor<parse_x_t, parse_num_t>> {};
// w += mem x M
// w -= mem x M
struct parse_mem_aop_read : pegtl::seq<parse_register, parse_spaces, parse_aop, parse_spaces, parse_mem,
                                       parse_spaces, parse_register, parse_spaces, parse_M> {};
// w <- t cmp t
struct parse_assign_cmp : pegtl::seq<parse_register, parse_spaces, parse_arrow, parse_spaces, pegtl::sor<parse_x_t, parse_num_t>, parse_spaces,
                                     parse_cmp, parse_spaces, pegtl::sor<parse_x_t, parse_num_t>> {};
// cjump t cmp t label
struct parse_label_inst_useful : parse_label {};
struct parse_cond_jump : pegtl::seq<parse_cjump, parse_spaces, pegtl::sor<parse_x_t, parse_num_t>, parse_spaces, parse_cmp, parse_spaces,
                                    pegtl::sor<parse_x_t, parse_num_t>, parse_spaces, parse_label_inst_useful> {};
// label
struct parse_label_inst : parse_label {};
// goto label
struct parse_goto_label : pegtl::seq<parse_goto, parse_spaces, parse_label_inst_useful> {};
// call u N
struct parse_w_u : parse_register {};
struct parse_l_u : parse_s_fname_val {};
struct parse_call_inst : pegtl::seq<parse_call, parse_spaces, pegtl::sor<parse_w_u, parse_l_u>, parse_spaces, parse_M> {};
// w @ w w E
struct parse_load_effective_addr : pegtl::seq<parse_register, parse_spaces, pegtl::string<'@'>, parse_spaces, parse_register, parse_spaces, parse_register, parse_spaces, parse_M> {};
struct parse_instructions : pegtl::seq<parse_seps,
                                       pegtl::plus<pegtl::seq<parse_spaces, pegtl::sor<parse_return, parse_call_print, parse_call_input, parse_call_allocate, parse_call_tuple_error, parse_call_tensor_error, parse_call_inst, parse_cond_jump, parse_label_inst, parse_memwrite_expression, parse_aop_expression, parse_sop_expression, parse_memread_expression, parse_assign_cmp, parse_assignment, parse_increment, parse_decrement, parse_mem_aop_read, parse_mem_aop_write, parse_goto_label, parse_load_effective_addr>, parse_seps>>> {};

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
struct action<parse_label_inst_useful> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_labels.push_back(label{in.string()});
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
struct action<parse_M> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_offset.push_back(N{std::stoll(in.string())});
  }
};
template <>
struct action<parse_l_u> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items_u.push_back(u{l{in.string()}});
  }
};
template <>
struct action<parse_w_u> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto reg = parsed_items.back();
    parsed_items.pop_back();
    parsed_items_u.push_back(u{std::get<r>(reg)});
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
struct action<parse_increment> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto target_reg = parsed_items.back();
    parsed_items.pop_back();
    auto newI = Increment{std::get<r>(target_reg)};
    program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_decrement> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto target_reg = parsed_items.back();
    parsed_items.pop_back();
    auto newI = Decrement{std::get<r>(target_reg)};
    program.functions.back().instructions.push_back(i{newI});
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
struct action<parse_sop> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    if (in.string() == "<<=") {
      parsed_sops.push_back(sop::lshift);
    } else if (in.string() == ">>=") {
      parsed_sops.push_back(sop::rshift);
    }
  }
};
template <>
struct action<parse_cmp> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    if (in.string() == "<") {
      parsed_cmps.push_back(cmp::lt);
    } else if (in.string() == "<=") {
      parsed_cmps.push_back(cmp::le);
    } else if (in.string() == "=") {
      parsed_cmps.push_back(cmp::eq);
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
struct action<parse_memread_expression> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto dst = parsed_items.back();
    parsed_items.pop_back();
    auto base = parsed_items.back();
    parsed_items.pop_back();
    auto offset = parsed_offset.back();
    parsed_offset.pop_back();
    auto newI = MemRead{std::get<r>(dst), std::get<r>(base), offset.value};
    program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_memwrite_expression> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto src = parsed_items.back();
    parsed_items.pop_back();
    auto base = parsed_items.back();
    parsed_items.pop_back();
    auto offset = parsed_offset.back();
    parsed_offset.pop_back();
    auto newI = MemWrite{std::get<r>(base), offset.value, src};
    program.functions.back().instructions.push_back(i{newI});
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
  }
};
template <>
struct action<parse_sop_expression> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto src_item = parsed_items_t.back();
    parsed_items_t.pop_back();
    bool reg = std::holds_alternative<r>(src_item);

    t src = reg
                ? t{std::get<r>(src_item)}
                : t{std::get<N>(src_item)};

    auto sop_op = parsed_sops.back();
    parsed_sops.pop_back();
    auto dst = parsed_items.back();
    parsed_items.pop_back();
    i newI;
    if (reg) {
      newI = SopReg{std::get<r>(dst), sop_op, std::get<r>(src)};
    } else {
      newI = SopNum{std::get<r>(dst), sop_op, std::get<N>(src)};
    }
    program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_assign_cmp> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto right_item = parsed_items_t.back();
    parsed_items_t.pop_back();
    auto cmp_op = parsed_cmps.back();
    parsed_cmps.pop_back();
    auto left_item = parsed_items_t.back();
    parsed_items_t.pop_back();
    auto dst = parsed_items.back();
    parsed_items.pop_back();
    i newI = Compare{std::get<r>(dst), left_item, cmp_op, right_item};
    program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_goto_label> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto label = parsed_labels.back();
    parsed_labels.pop_back();
    i newI = Goto{label};
    program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_call_inst> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto num_args = parsed_offset.back();
    parsed_offset.pop_back();
    auto callee = parsed_items_u.back();
    parsed_items_u.pop_back();
    i newI = Call{callee, num_args};
    program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_cond_jump> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto right_item = parsed_items_t.back();
    parsed_items_t.pop_back();
    auto cmp_op = parsed_cmps.back();
    parsed_cmps.pop_back();
    auto left_item = parsed_items_t.back();
    parsed_items_t.pop_back();
    auto label = parsed_labels.back();
    parsed_labels.pop_back();
    i newI = Cjump{left_item, cmp_op, right_item, label};
    program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_mem_aop_read> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto base = parsed_items.back();
    parsed_items.pop_back();
    auto dest = parsed_items.back();
    parsed_items.pop_back();
    auto aop = parsed_ops.back();
    parsed_ops.pop_back();
    auto offset = parsed_offset.back();
    parsed_offset.pop_back();
    auto newI = MemAopRead{std::get<r>(dest), aop, std::get<r>(base), offset.value};
    program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_mem_aop_write> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto src_item = parsed_items_t.back();
    parsed_items_t.pop_back();
    t src = std::holds_alternative<r>(src_item)
                ? t{std::get<r>(src_item)}
                : t{std::get<N>(src_item)};
    auto aop = parsed_ops.back();
    parsed_ops.pop_back();
    auto offset = parsed_offset.back();
    parsed_offset.pop_back();
    auto base = parsed_items.back();
    parsed_items.pop_back();
    auto newI = MemAopWrite{std::get<r>(base), offset.value, aop, src};
    program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_load_effective_addr> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto scale = parsed_offset.back();
    parsed_offset.pop_back();
    auto index = parsed_items.back();
    parsed_items.pop_back();
    auto base = parsed_items.back();
    parsed_items.pop_back();
    auto dest = parsed_items.back();
    parsed_items.pop_back();
    i newI = LoadAddr{std::get<r>(dest), std::get<r>(base), std::get<r>(index), scale.value};
    program.functions.back().instructions.push_back(newI);
  };
};
} // namespace L1