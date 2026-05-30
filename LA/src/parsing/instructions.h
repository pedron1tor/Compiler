#pragma once

#include <IR.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include "primitives.h"
#include "registers.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace LA {

// var <- s
struct parse_assignment : pegtl::seq<parse_name, parse_spaces, parse_arrow, parse_spaces, parse_t> {};

// op ::= + | - | * | & | << | >> | < | <= | = | >= | >
struct parse_binop_op : pegtl::sor<parse_aop, parse_cmp> {};
// var <- t op t
struct parse_binop : pegtl::seq<
                         pegtl::at<pegtl::seq<parse_name, parse_spaces, parse_arrow, parse_spaces,
                                              parse_t, parse_spaces, parse_binop_op>>,
                         parse_name, parse_spaces, parse_arrow, parse_spaces,
                         parse_t, parse_spaces, parse_binop_op, parse_spaces, parse_t> {};

// args ::= t | t (, t)*
struct parse_args_parse_t : parse_t {};
struct parse_args : pegtl::seq<
                        pegtl::one<'('>, parse_seps,
                        pegtl::opt<pegtl::seq<
                            parse_args_parse_t,
                            pegtl::star<pegtl::seq<parse_seps, parse_comma, parse_seps, parse_args_parse_t>>>>,
                        parse_seps, pegtl::one<')'>> {};
// callee ::= u | print | input | tuple-error | tensor-error
struct parse_call_print : parse_print {};
struct parse_call_input : parse_input {};
struct parse_call_tuple_error : parse_tuple_error {};
struct parse_call_tensor_error : parse_tensor_error {};
struct parse_callee : parse_name {};

// call callee ( args? )
struct parse_call_callee : pegtl::seq<parse_callee, parse_seps, parse_args> {};
struct parse_call_callee_dest : parse_variable {};
// var <- call callee ( args? )
struct parse_assign_call_callee : pegtl::seq<
                                      parse_call_callee_dest, parse_spaces, parse_arrow, parse_spaces, parse_callee, parse_seps, parse_args> {};

// array idxing
struct parse_array_index_t : parse_t {};
struct parse_array_index : pegtl::seq<
                               pegtl::one<'['>, parse_seps, parse_array_index_t, parse_seps, pegtl::one<']'>> {};

// var <- var([t])+
struct parse_array_read_dest : parse_variable {};
struct parse_array_read_src : parse_variable {};
struct parse_array_read : pegtl::seq<
                              pegtl::at<pegtl::seq<parse_variable, parse_spaces, parse_arrow,
                                                   parse_spaces, parse_variable, parse_spaces, pegtl::one<'['>>>,
                              parse_array_read_dest, parse_spaces, parse_arrow, parse_spaces,
                              parse_array_read_src, pegtl::plus<parse_array_index>> {};

// var([t])+ <- t
struct parse_array_write_dest : parse_variable {};
struct parse_array_write : pegtl::seq<
                               pegtl::at<pegtl::seq<parse_variable, pegtl::one<'['>>>,
                               parse_array_write_dest, pegtl::plus<parse_array_index>,
                               parse_spaces, parse_arrow, parse_spaces, parse_t> {};

// var <- length var t
struct parse_length_dest : parse_variable {};
struct parse_length_src : parse_variable {};
struct parse_length_dim_t : parse_t {};
struct parse_length_with_dim : pegtl::seq<
                                   pegtl::at<pegtl::seq<parse_variable, parse_spaces, parse_arrow, parse_spaces,
                                                        parse_length, parse_spaces, parse_variable, parse_spaces, parse_t>>,
                                   parse_length_dest, parse_spaces, parse_arrow, parse_spaces,
                                   parse_length, parse_spaces, parse_length_src, parse_spaces, parse_length_dim_t> {};
// var <- length var
struct parse_length_no_dim_dest : parse_variable {};
struct parse_length_no_dim_src : parse_variable {};
struct parse_length_no_dim : pegtl::seq<
                                 parse_length_no_dim_dest, parse_spaces, parse_arrow, parse_spaces,
                                 parse_length, parse_spaces, parse_length_no_dim_src> {};
// var <- length var t | var <- length var
struct parse_length_inst : pegtl::sor<parse_length_with_dim, parse_length_no_dim> {};

// var <- new Array(args)
struct parse_new_array_dest : parse_variable {};
struct parse_new_array : pegtl::seq<
                             parse_new_array_dest, parse_spaces, parse_arrow, parse_spaces,
                             parse_new, parse_spaces, parse_array_constructor, parse_seps, parse_args> {};

// var <- new Tuple(t)
struct parse_new_tuple_dest : parse_variable {};
struct parse_new_tuple_size_t : parse_t {};
struct parse_new_tuple : pegtl::seq<
                             parse_new_tuple_dest, parse_spaces, parse_arrow, parse_spaces,
                             parse_new, parse_spaces, parse_tuple_constructor, parse_seps,
                             pegtl::one<'('>, parse_seps, parse_new_tuple_size_t, parse_seps, pegtl::one<')'>> {};

// type var
struct parse_typevar_type : parse_type {};
struct parse_typevar_var : parse_variable {};
struct parse_typevar_decl : pegtl::seq<parse_typevar_type, parse_spaces, parse_typevar_var> {};
/*
 terminal instructions
*/
// te ::= br label
struct parse_br_target : parse_label {};
struct parse_brlabel_inst : pegtl::seq<parse_br, parse_spaces, parse_br_target> {};
// te ::= br t label label
struct parse_brcond_true : parse_label {};
struct parse_brcond_false : parse_label {};
struct parse_brcond_inst : pegtl::seq<
                               parse_br, parse_spaces, parse_t,
                               parse_spaces, parse_brcond_true,
                               parse_spaces, parse_brcond_false> {};
// te ::= return | return t
struct parse_return_inst : parse_return {};
struct parse_return_t : pegtl::seq<parse_return, parse_seps, parse_t> {};

// label
struct parse_label_inst : parse_label {
};

struct parse_i : pegtl::sor<
                     parse_assign_call_callee,
                     parse_call_callee,
                     parse_array_read,
                     parse_array_write,
                     parse_length_inst,
                     parse_new_array,
                     parse_new_tuple,
                     parse_brcond_inst,
                     parse_brlabel_inst,
                     parse_return_t,
                     parse_return_inst,
                     parse_label_inst,
                     parse_binop,
                     parse_assignment,
                     parse_typevar_decl> {};

struct parse_instructions : pegtl::star<pegtl::seq<parse_seps, parse_i>> {};

/*
 * Actions
 */
template <>
struct action<parse_label_inst> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    L3::label lbl = L3::label{in.string()};
    acc.program.functions.back().instructions.push_back(L3::LabelInst{lbl}); // whenever a label is encountered in the wild, createa new block
  }
};

// terminal instructions
template <>
struct action<parse_return_inst> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    acc.program.functions.back().instructions.push_back(L3::Return{});
  }
};
template <>
struct action<parse_return_t> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto val = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    acc.program.functions.back().instructions.push_back(LA::ReturnT{val});
  }
};
template <>
struct action<parse_br_target> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_labels.push_back(L3::label{in.string()});
  }
};
template <>
struct action<parse_brlabel_inst> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto lbl = acc.parsed_labels.back();
    acc.parsed_labels.pop_back();
    acc.program.functions.back().instructions.push_back(L3::brlabel{lbl});
  };
};
template <>
struct action<parse_brcond_true> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_labels.push_back(L3::label{in.string()});
  }
};
template <>
struct action<parse_brcond_false> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_labels.push_back(L3::label{in.string()});
  }
};
template <>
struct action<parse_brcond_inst> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto false_lbl = acc.parsed_labels.back().name;
    acc.parsed_labels.pop_back();
    auto true_lbl = acc.parsed_labels.back().name;
    acc.parsed_labels.pop_back();
    auto cond = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    acc.program.functions.back().instructions.push_back(LA::BrCond{cond, true_lbl, false_lbl});
  }
};

// binop
template <>
struct action<parse_binop_op> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto str = in.string();
    auto operation = str_to_op(str);
    acc.parsed_ops.push_back(operation);
  }
};
template <>
struct action<parse_binop> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto right = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto operation = acc.parsed_ops.back();
    acc.parsed_ops.pop_back();
    auto left = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    acc.program.functions.back().instructions.push_back(i{BinOp{dest, left, operation, right}});
  }
};

template <>
struct action<parse_assignment> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto src = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto dst = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    acc.program.functions.back().instructions.push_back(i{LA::Assign{dst, src}});
  }
};

// calls
template <>
struct action<parse_args_parse_t> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    acc.parsed_call_args.push_back(acc.parsed_items_t.back());
    acc.parsed_items_t.pop_back();
  }
};

template <>
struct action<parse_call_callee_dest> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_callee> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_call_callee> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto callee = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto args = std::vector<LA::t>(acc.parsed_call_args.begin(), acc.parsed_call_args.end());
    acc.parsed_call_args.clear();
    acc.program.functions.back().instructions.push_back(i{LA::CallArbitrary{callee, args}});
  }
};
template <>
struct action<parse_assign_call_callee> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto callee = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto args = std::vector<LA::t>(acc.parsed_call_args.begin(), acc.parsed_call_args.end());
    acc.parsed_call_args.clear();
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    acc.program.functions.back().instructions.push_back(i{LA::CallAssignArbitrary{dest, LA::CallArbitrary{callee, args}}});
  }
};

// array indexing
template <>
struct action<parse_array_index_t> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    acc.parsed_indices.push_back(acc.parsed_items_t.back());
    acc.parsed_items_t.pop_back();
  }
};
template <>
struct action<parse_array_read_dest> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_array_read_src> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_array_read> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto src = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto indices = std::vector<LA::t>(acc.parsed_indices.begin(), acc.parsed_indices.end());
    acc.parsed_indices.clear();
    acc.program.functions.back().instructions.push_back(i{ArrayRead{dest, src, indices, (int64_t)in.position().line}});
  }
};
template <>
struct action<parse_array_write_dest> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_array_write> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    auto src = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto indices = std::vector<LA::t>(acc.parsed_indices.begin(), acc.parsed_indices.end());
    acc.parsed_indices.clear();
    acc.program.functions.back().instructions.push_back(i{LA::ArrayWrite{dest, indices, src, (int64_t)in.position().line}});
  }
};

// length
template <>
struct action<parse_length_dest> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_length_src> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_length_with_dim> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto dim = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto src = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    acc.program.functions.back().instructions.push_back(i{LA::Length{dest, src, std::optional<LA::t>{dim}}});
  }
};
template <>
struct action<parse_length_no_dim_dest> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_length_no_dim_src> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_length_no_dim> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto src = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    acc.program.functions.back().instructions.push_back(i{Length{dest, src, std::nullopt}});
  }
};

// new Array / new Tuple
template <>
struct action<parse_new_array_dest> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_new_array> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto dims = std::vector<LA::t>(acc.parsed_call_args.begin(), acc.parsed_call_args.end());
    acc.parsed_call_args.clear();
    acc.program.functions.back().instructions.push_back(i{LA::NewArray{dest, dims}});
  }
};
template <>
struct action<parse_new_tuple_dest> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_new_tuple> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto size = acc.parsed_items_t.back();
    acc.parsed_items_t.pop_back();
    auto dest = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    acc.program.functions.back().instructions.push_back(i{LA::NewTuple{dest, size}});
  }
};

// type var declaration
template <>
struct action<parse_typevar_var> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) { acc.parsed_names.push_back(in.string()); }
};
template <>
struct action<parse_typevar_decl> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    auto var_name = acc.parsed_names.back();
    acc.parsed_names.pop_back();
    auto var_type = acc.parsed_types.back();
    acc.parsed_types.pop_back();
    acc.program.functions.back().instructions.push_back(i{LA::TypeVar{var_type, var_name}});
  }
};

} // namespace LA