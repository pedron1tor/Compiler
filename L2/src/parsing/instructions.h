#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include "primitives.h"
#include "registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
struct parse_call_print : pegtl::seq<parse_call, parse_spaces, parse_print, parse_spaces, parse_number> {};
struct parse_call_input : pegtl::seq<parse_call, parse_spaces, parse_input, parse_spaces, parse_number> {};
struct parse_call_allocate : pegtl::seq<parse_call, parse_spaces, parse_allocate, parse_spaces, parse_number> {};
struct parse_call_tuple_error : pegtl::seq<parse_call, parse_spaces, parse_tuple_error, parse_spaces, parse_number> {};
struct parse_tensor_error_F : parse_number {};
struct parse_increment : pegtl::seq<parse_w, parse_spaces, pegtl::string<'+', '+'>> {};
struct parse_decrement : pegtl::seq<parse_w, parse_spaces, pegtl::string<'-', '-'>> {};
struct parse_call_tensor_error : pegtl::seq<parse_call, parse_spaces, parse_tensor_error, parse_spaces, parse_tensor_error_F> {};
// w <- s
struct parse_assignment : pegtl::seq<
                              parse_w,
                              parse_spaces,
                              parse_arrow,
                              parse_spaces,
                              parse_s> {};
// w <- mem x M
struct parse_M : parse_number {};
struct parse_memread_expression : pegtl::seq<parse_w,
                                             parse_spaces, parse_arrow, parse_spaces, parse_mem, parse_spaces, parse_w, parse_spaces, parse_M> {};
// mem x M <- s
struct parse_memwrite_expression : pegtl::seq<parse_mem, parse_spaces, parse_w, parse_spaces, parse_M, parse_spaces, parse_arrow, parse_spaces, parse_s> {};
// w aop t
struct parse_aop_expression : pegtl::seq<parse_w, parse_spaces, parse_aop, parse_spaces, parse_t> {};
// w sop sx
// w sop N
struct parse_sop_expression : pegtl::seq<parse_w, parse_spaces, parse_sop, parse_spaces, parse_t> {};
// mem x M += t
// mem x M -= t
struct parse_mem_aop_write : pegtl::seq<parse_mem, parse_spaces, parse_w, parse_spaces, parse_M, parse_spaces, parse_aop, parse_spaces,
                                        parse_t> {};
// w += mem x M
// w -= mem x M
struct parse_mem_aop_read : pegtl::seq<parse_w, parse_spaces, parse_aop, parse_spaces, parse_mem,
                                       parse_spaces, parse_w, parse_spaces, parse_M> {};
// w <- t cmp t
struct parse_assign_cmp : pegtl::seq<parse_w, parse_spaces, parse_arrow, parse_spaces, parse_t, parse_spaces,
                                     parse_cmp, parse_spaces, parse_t> {};
// cjump t cmp t label
struct parse_label_inst_useful : parse_label {};
struct parse_cond_jump : pegtl::seq<parse_cjump, parse_spaces, parse_t, parse_spaces, parse_cmp, parse_spaces,
                                    parse_t, parse_spaces, parse_label_inst_useful> {};
// label
struct parse_label_inst : parse_label {};
// goto label
struct parse_goto_label : pegtl::seq<parse_goto, parse_spaces, parse_label_inst_useful> {};
// call u N
struct parse_call_inst : pegtl::seq<parse_call, parse_spaces, parse_u, parse_spaces, parse_M> {};
// w @ w w E
struct parse_load_effective_addr : pegtl::seq<parse_w, parse_spaces, pegtl::string<'@'>, parse_spaces, parse_w, parse_spaces, parse_w, parse_spaces, parse_M> {};
// w <- stack-arg M
struct parse_stack_arg_inst : pegtl::seq<parse_w, parse_spaces, parse_arrow, parse_spaces, parse_stack_arg, parse_spaces, parse_M> {
};
struct parse_instructions : pegtl::seq<parse_seps,
                                       pegtl::plus<pegtl::seq<parse_spaces, pegtl::sor<parse_return, parse_call_print, parse_call_input, parse_call_allocate, parse_call_tuple_error, parse_call_tensor_error, parse_call_inst, parse_cond_jump, parse_label_inst, parse_memwrite_expression, parse_aop_expression, parse_sop_expression, parse_memread_expression, parse_assign_cmp, parse_assignment, parse_increment, parse_decrement, parse_mem_aop_read, parse_mem_aop_write, parse_goto_label, parse_load_effective_addr, parse_stack_arg_inst>, parse_seps>>> {};

/*
templates
*/
template <typename Rule, typename Inst>
struct action_push {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto &currentF = acc.program.functions.back();
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
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_t.push_back(t{N{std::stoll(in.string())}});
  }
};
template <>
struct action<parse_call_tensor_error> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto F = std::get<N>(acc.parsed_items_t.back()).value;
    acc.parsed_items_t.pop_back();
    acc.program.functions.back().instructions.push_back(i{CallTensorError{F}});
  }
};

template <>
struct action<parse_label_inst> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.program.functions.back().instructions.push_back(i{LabelInst{label{in.string()}}});
  }
};
template <>
struct action<parse_label_inst_useful> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_labels.push_back(label{in.string()});
  }
};

template <>
struct action<parse_M> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_offset.push_back(N{std::stoll(in.string())});
  }
};

template <>
struct action<parse_increment> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto target_reg = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto newI = Increment{target_reg};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_decrement> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto target_reg = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto newI = Decrement{target_reg};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_aop> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    if (in.string() == "+=") {
      acc.parsed_ops.push_back(aop::add);
    } else if (in.string() == "-=") {
      acc.parsed_ops.push_back(aop::sub);
    } else if (in.string() == "*=") {
      acc.parsed_ops.push_back(aop::mul);
    } else if (in.string() == "&=") {
      acc.parsed_ops.push_back(aop::and_);
    }
  }
};
template <>
struct action<parse_sop> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    if (in.string() == "<<=") {
      acc.parsed_sops.push_back(sop::lshift);
    } else if (in.string() == ">>=") {
      acc.parsed_sops.push_back(sop::rshift);
    }
  }
};
template <>
struct action<parse_cmp> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    if (in.string() == "<") {
      acc.parsed_cmps.push_back(cmp::lt);
    } else if (in.string() == "<=") {
      acc.parsed_cmps.push_back(cmp::le);
    } else if (in.string() == "=") {
      acc.parsed_cmps.push_back(cmp::eq);
    }
  }
};
template <>
struct action<parse_memread_expression> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto base = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto dst = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto offset = acc.parsed_offset.back();
    acc.parsed_offset.pop_back();
    auto newI = MemRead{dst, base, offset.value};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_memwrite_expression> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto src = acc.parsed_items.back();
    acc.parsed_items.pop_back();
    auto base = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto offset = acc.parsed_offset.back();
    acc.parsed_offset.pop_back();
    auto newI = MemWrite{base, offset.value, src};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_assignment> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto src = acc.parsed_items.back();
    acc.parsed_items.pop_back();
    auto dst = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto newI = Assign{dst, src};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_aop_expression> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto src_item = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto aop = acc.parsed_ops.back();
    acc.parsed_ops.pop_back();
    auto dst = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto newI = AopInst{dst, aop, src_item};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_sop_expression> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto src = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto sop_op = acc.parsed_sops.back();
    acc.parsed_sops.pop_back();
    auto dst = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    i newI;
    if (std::holds_alternative<N>(src)) {
      newI = SopNum{dst, sop_op, std::get<N>(src)};
    } else {
      newI = SopReg{dst, sop_op, src};
    }
    acc.program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_assign_cmp> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto right_item = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto cmp_op = acc.parsed_cmps.back();
    acc.parsed_cmps.pop_back();
    auto left_item = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto dst = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    i newI = Compare{dst, left_item, cmp_op, right_item};
    acc.program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_goto_label> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto label = acc.parsed_labels.back();
    acc.parsed_labels.pop_back();
    i newI = Goto{label};
    acc.program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_call_inst> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto num_args = acc.parsed_offset.back();
    acc.parsed_offset.pop_back();
    auto callee = acc.parsed_items_u.back();
    acc.parsed_items_u.pop_back();
    i newI = Call{callee, num_args};
    acc.program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_cond_jump> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto right_item = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto cmp_op = acc.parsed_cmps.back();
    acc.parsed_cmps.pop_back();
    auto left_item = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto label = acc.parsed_labels.back();
    acc.parsed_labels.pop_back();
    i newI = Cjump{left_item, cmp_op, right_item, label};
    acc.program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_mem_aop_read> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto base = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto dest = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto aop = acc.parsed_ops.back();
    acc.parsed_ops.pop_back();
    auto offset = acc.parsed_offset.back();
    acc.parsed_offset.pop_back();
    auto newI = MemAopRead{dest, aop, base, offset.value};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_mem_aop_write> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto src_item = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    t src = std::holds_alternative<r>(src_item)
                ? t{std::get<r>(src_item)}
                : t{std::get<N>(src_item)};
    auto aop = acc.parsed_ops.back();
    acc.parsed_ops.pop_back();
    auto offset = acc.parsed_offset.back();
    acc.parsed_offset.pop_back();
    auto base = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto newI = MemAopWrite{base, offset.value, aop, src};
    acc.program.functions.back().instructions.push_back(i{newI});
  }
};
template <>
struct action<parse_load_effective_addr> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto scale = acc.parsed_offset.back();
    acc.parsed_offset.pop_back();
    auto index = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto base = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    auto dest = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    i newI = LoadAddr{dest, base, index, scale.value};
    acc.program.functions.back().instructions.push_back(newI);
  };
};
template <>
struct action<parse_stack_arg_inst> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto num_args = acc.parsed_offset.back();
    acc.parsed_offset.pop_back();
    auto arg = acc.parsed_items_w.back();
    acc.parsed_items_w.pop_back();
    i newI = StackArg{arg, num_args};
    acc.program.functions.back().instructions.push_back(newI);
  };
};
} // namespace L2