#pragma once

#include <L3.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include "primitives.h"
#include "registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L3
{

  // w <- s
  struct parse_assignment : pegtl::seq<
                                parse_w,
                                parse_spaces,
                                parse_arrow,
                                parse_spaces,
                                parse_s>
  {
  };

  // w <- t cmp t
  struct parse_assign_cmp : pegtl::seq<parse_w, parse_spaces, parse_arrow, parse_spaces, parse_t, parse_spaces,
                                       parse_cmp, parse_spaces, parse_t>
  {
  };

  // label
  struct parse_label_inst : parse_label
  {
  };
  struct parse_return_inst : parse_return
  {
  };
  // return t
  struct parse_return_t : pegtl::seq<parse_return, parse_seps, parse_t>
  {
  };
  struct parse_args_parse_t : parse_t
  {
  };
  struct parse_args : pegtl::seq<
                          pegtl::one<'('>,
                          parse_seps,
                          pegtl::opt<pegtl::seq<
                              parse_args_parse_t,
                              pegtl::star<pegtl::seq<parse_seps, parse_comma, parse_seps, parse_args_parse_t>>>>,
                          parse_seps,
                          pegtl::one<')'>>
  {
  };
  // parse callees
  struct parse_call_print : parse_print
  {
  };
  struct parse_call_input : parse_input
  {
  };
  struct parse_call_allocate : parse_allocate
  {
  };
  struct parse_call_tuple_error : parse_tuple_error
  {
  };
  struct parse_tensor_error_F : parse_number
  {
  };

  struct parse_call_tensor_error : parse_tensor_error
  {
  };
  struct parse_callee_u : parse_u
  {
  };
  struct parse_callee : pegtl::sor<
                            parse_call_print, parse_call_input, parse_call_allocate,
                            parse_call_tuple_error, parse_call_tensor_error, parse_callee_u>
  {
  };
  // call callee ( args )
  struct parse_call_callee : pegtl::seq<parse_call, parse_seps, parse_callee, parse_seps, parse_args>
  {
  };
  struct parse_call_callee_dest : parse_variable
  {
  };
  // var <- call callee ( args )
  struct parse_assign_call_callee : pegtl::seq<
                                        parse_call_callee_dest, parse_spaces, parse_arrow, parse_spaces,
                                        parse_call, parse_seps, parse_callee, parse_seps, parse_args>
  {
  };

  // br :label
  struct parse_br_target : parse_label
  {
  };
  struct parse_brlabel_inst : pegtl::seq<parse_br, parse_spaces, parse_br_target>
  {
  };
  // br t :label
  struct parse_brtlabel_inst : pegtl::seq<parse_br, parse_spaces, parse_t, parse_spaces, parse_br_target>
  {
  };
  // %dest <- load %src
  struct parse_load_dest : parse_variable
  {
  };
  struct parse_load_src : parse_variable
  {
  };
  struct parse_load_l3 : pegtl::seq<parse_load_dest, parse_spaces, parse_arrow, parse_spaces, parse_load, parse_spaces, parse_load_src>
  {
  };
  // store %var <- s
  struct parse_store_dest : parse_variable
  {
  };
  struct parse_store_l3 : pegtl::seq<parse_store, parse_spaces, parse_store_dest, parse_spaces, parse_arrow, parse_spaces, parse_s>
  {
  };
  // w <- t aop t
  struct parse_arith_l3_op : parse_aop
  {
  };
  struct parse_arith_l3 : pegtl::seq<
                              pegtl::at<pegtl::seq<parse_w, parse_spaces, parse_arrow, parse_spaces, parse_t, parse_spaces, parse_aop>>,
                              parse_w,
                              parse_spaces, parse_arrow, parse_spaces,
                              parse_t,
                              parse_spaces,
                              parse_arith_l3_op,
                              parse_spaces,
                              parse_t>
  {
  };

  struct parse_instructions : pegtl::seq<parse_seps,
                                         pegtl::plus<pegtl::seq<parse_spaces, pegtl::sor<parse_assign_call_callee, parse_call_callee, parse_brtlabel_inst, parse_brlabel_inst, parse_return_t, parse_return_inst, parse_call_print, parse_call_input, parse_call_allocate, parse_call_tuple_error, parse_call_tensor_error, parse_label_inst, parse_assign_cmp, parse_arith_l3, parse_load_l3, parse_store_l3, parse_assignment>, parse_seps>>>
  {
  };

  /*
  templates
  */
  template <typename Rule, typename Inst>
  struct action_push
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      auto &currentF = acc.program.functions.back();
      currentF.instructions.push_back(i{Inst{}});
    }
  };
  template <>
  struct action<parse_return_inst> : action_push<parse_return_inst, Return>
  {
  };
  template <>
  struct action<parse_label_inst>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.program.functions.back().instructions.push_back(i{LabelInst{label{in.string()}}});
    }
  };

  template <>
  struct action<parse_arith_l3_op>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      if (in.string() == "+")
        acc.parsed_ops.push_back(aop::add);
      else if (in.string() == "-")
        acc.parsed_ops.push_back(aop::sub);
      else if (in.string() == "*")
        acc.parsed_ops.push_back(aop::mul);
      else if (in.string() == "&")
        acc.parsed_ops.push_back(aop::and_);
      else if (in.string() == "<<")
        acc.parsed_ops.push_back(aop::lshift);
      else if (in.string() == ">>")
        acc.parsed_ops.push_back(aop::rshift);
    }
  };

  template <>
  struct action<parse_cmp>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      if (in.string() == "<")
      {
        acc.parsed_cmps.push_back(cmp::lt);
      }
      else if (in.string() == ">")
      {
        acc.parsed_cmps.push_back(cmp::gt);
      }
      else if (in.string() == "<=")
      {
        acc.parsed_cmps.push_back(cmp::le);
      }
      else if (in.string() == ">=")
      {
        acc.parsed_cmps.push_back(cmp::gte);
      }
      else if (in.string() == "=")
      {
        acc.parsed_cmps.push_back(cmp::eq);
      }
    }
  };

  template <>
  struct action<parse_assignment>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      auto src = acc.parsed_items.back();
      acc.parsed_items.pop_back();
      auto dst = acc.parsed_items_w.back();
      acc.parsed_items_w.pop_back();
      auto newI = Assign{dst, src};
      acc.program.functions.back().instructions.push_back(i{newI});
    }
  };

  template <>
  struct action<parse_assign_cmp>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
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
  struct action<parse_return_t>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      auto t = acc.parsed_items_t.back();
      acc.parsed_items_t.pop_back();
      i newI = ReturnT{t};
      acc.program.functions.back().instructions.push_back(newI);
    };
  };
  template <>
  struct action<parse_args_parse_t>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_call_args.push_back(acc.parsed_items_t.back());
      acc.parsed_items_t.pop_back();
    }
  };
  template <>
  struct action<parse_call_print>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      acc.parsed_callee.push_back(CallPrint{});
    }
  };
  template <>
  struct action<parse_call_input>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      acc.parsed_callee.push_back(CallInput{});
    }
  };
  template <>
  struct action<parse_call_allocate>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      acc.parsed_callee.push_back(CallAllocate{});
    }
  };
  template <>
  struct action<parse_call_tuple_error>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      acc.parsed_callee.push_back(CallTupleError{});
    }
  };
  template <>
  struct action<parse_call_tensor_error>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      acc.parsed_callee.push_back(CallTensorError{});
    }
  };
  template <>
  struct action<parse_callee_u>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      acc.parsed_callee.push_back(acc.parsed_items_u.back());
      acc.parsed_items_u.pop_back();
    }
  };
  template <>
  struct action<parse_call_callee_dest>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_w.push_back(w{var{remove_perc(in.string())}});
    }
  };

  template <>
  struct action<parse_call_callee>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      auto callee = acc.parsed_callee.back();
      acc.parsed_callee.pop_back();
      auto args = std::vector<t>(acc.parsed_call_args.begin(), acc.parsed_call_args.end());
      acc.parsed_call_args.clear();
      acc.program.functions.back().instructions.push_back(i{CallL3{callee, args}});
    }
  };

  template <>
  struct action<parse_assign_call_callee>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      auto callee = acc.parsed_callee.back();
      acc.parsed_callee.pop_back();
      auto args = std::vector<t>(acc.parsed_call_args.begin(), acc.parsed_call_args.end());
      acc.parsed_call_args.clear();
      auto dest = std::get<var>(acc.parsed_items_w.back());
      acc.parsed_items_w.pop_back();
      acc.program.functions.back().instructions.push_back(i{CallAssignL3{dest, CallL3{callee, args}}});
    }
  };
  template <>
  struct action<parse_br_target>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_labels.push_back(label{in.string()});
    }
  };
  template <>
  struct action<parse_brlabel_inst>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      auto lbl = acc.parsed_labels.back();
      acc.parsed_labels.pop_back();
      acc.program.functions.back().instructions.push_back(i{brlabel{l{lbl.name}}});
    }
  };
  template <>
  struct action<parse_brtlabel_inst>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      auto lbl = acc.parsed_labels.back();
      acc.parsed_labels.pop_back();
      auto cond = acc.parsed_items_t.back();
      acc.parsed_items_t.pop_back();
      acc.program.functions.back().instructions.push_back(i{brtlabel{cond, l{lbl.name}}});
    }
  };
  template <>
  struct action<parse_arith_l3>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      auto right = acc.parsed_items_t.back();
      acc.parsed_items_t.pop_back();
      auto op = acc.parsed_ops.back();
      acc.parsed_ops.pop_back();
      auto left = acc.parsed_items_t.back();
      acc.parsed_items_t.pop_back();
      auto dest = acc.parsed_items_w.back();
      acc.parsed_items_w.pop_back();
      acc.program.functions.back().instructions.push_back(i{AopL3{dest, left, op, right}});
    }
  };
  template <>
  struct action<parse_load_dest>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_w.push_back(w{var{remove_perc(in.string())}});
    }
  };
  template <>
  struct action<parse_load_src>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_w.push_back(w{var{remove_perc(in.string())}});
    }
  };
  template <>
  struct action<parse_load_l3>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      auto src = std::get<var>(acc.parsed_items_w.back());
      acc.parsed_items_w.pop_back();
      auto dest = std::get<var>(acc.parsed_items_w.back());
      acc.parsed_items_w.pop_back();
      acc.program.functions.back().instructions.push_back(i{LoadL3{dest, src}});
    }
  };
  template <>
  struct action<parse_store_dest>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_w.push_back(w{var{remove_perc(in.string())}});
    }
  };
  template <>
  struct action<parse_store_l3>
  {
    template <typename Input>
    static void apply(const Input &, GlobalAccumulator &acc)
    {
      auto src = acc.parsed_items.back();
      acc.parsed_items.pop_back();
      auto dest = std::get<var>(acc.parsed_items_w.back());
      acc.parsed_items_w.pop_back();
      acc.program.functions.back().instructions.push_back(i{StoreL3{dest, src}});
    }
  };
} // namespace L3