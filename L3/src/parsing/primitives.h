#pragma once

#include <L3.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L3
{
  /*
  tokens parsed
  */
  struct GlobalAccumulator
  {
    pl3 program;
    std::vector<s> parsed_items;
    std::vector<t> parsed_items_t;
    std::vector<aop> parsed_ops;
    std::vector<sop> parsed_sops;
    std::vector<cmp> parsed_cmps;
    std::vector<N> parsed_offset;
    std::vector<label> parsed_labels;
    std::vector<u> parsed_items_u;
    std::vector<w> parsed_items_w;
    std::vector<var> parsed_spill_args;
    std::vector<callee_type> parsed_callee;
    std::vector<t> parsed_call_args;
  };

  struct parse_number : pegtl::seq<pegtl::sor<
                            pegtl::seq<
                                pegtl::opt<pegtl::sor<pegtl::one<'+'>, pegtl::one<'-'>>>,
                                pegtl::range<'1', '9'>,
                                pegtl::star<pegtl::digit>>,
                            pegtl::one<'0'>>>
  {
  };
  struct parse_name : pegtl::seq<
                          pegtl::plus<
                              pegtl::sor<
                                  pegtl::alpha,
                                  pegtl::one<'_'>>>,
                          pegtl::star<
                              pegtl::sor<
                                  pegtl::alpha,
                                  pegtl::one<'_'>,
                                  pegtl::digit>>>
  {
  };
  struct parse_variable : pegtl::seq<pegtl::one<'%'>, parse_name>
  {
  };
  struct parse_label : pegtl::seq<pegtl::one<':'>, parse_name>
  {
  };
  struct parse_comments : pegtl::seq<pegtl::string<'/', '/'>, pegtl::until<pegtl::eolf>>
  {
  };
  /*
   * Separators.
   */
  struct parse_spaces : pegtl::star<pegtl::sor<pegtl::one<' '>, pegtl::one<'\t'>>>
  {
  };
  struct parse_seps : pegtl::star<pegtl::sor<pegtl::space, parse_comments>>
  {
  };
  struct parse_nochar : pegtl::not_at<pegtl::alnum>
  {
  };
  /*
  aop
  sop
  cmp
  */
  struct parse_plus_assign : pegtl::string<'+', '='>
  {
  };
  struct parse_minus_assign : pegtl::string<'-', '='>
  {
  };
  struct parse_mult_assign : pegtl::string<'*', '='>
  {
  };
  struct parse_and_assign : pegtl::string<'&', '='>
  {
  };

  struct parse_left_shift_assign : pegtl::string<'<', '<', '='>
  {
  };
  struct parse_right_shift_assign : pegtl::string<'>', '>', '='>
  {
  };
  struct parse_lt : pegtl::string<'<'>
  {
  };
  struct parse_gt : pegtl::string<'>'>
  {
  };
  struct parse_lte : pegtl::string<'<', '='>
  {
  };
  struct parse_eq : pegtl::string<'='>
  {
  };
  struct parse_aop_assign : pegtl::sor<parse_plus_assign, parse_minus_assign, parse_mult_assign,
                                       parse_and_assign>
  {
  };
  struct parse_plus : pegtl::seq<pegtl::one<'+'>>
  {
  };
  struct parse_minus : pegtl::seq<pegtl::one<'-'>>
  {
  };
  struct parse_mult : pegtl::seq<pegtl::one<'*'>>
  {
  };
  struct parse_and : pegtl::seq<pegtl::one<'&'>>
  {
  };
  struct parse_rshift : pegtl::seq<pegtl::string<'>', '>'>>
  {
  };
  struct parse_lshift : pegtl::seq<pegtl::string<'<', '<'>>
  {
  };
  struct parse_aop : pegtl::sor<parse_plus, parse_minus, parse_mult, parse_and, parse_rshift, parse_lshift>
  {
  };
  struct parse_cmp : pegtl::sor<parse_lte, parse_lt, parse_eq, parse_gt>
  {
  };
  /*
  keywords
  return
  call
  print
  input
  allocate
  tuple-error
  tensor-error
  stack-arg
  define
  ,
  */
  struct parse_return : pegtl::seq<pegtl::string<'r', 'e', 't', 'u', 'r', 'n'>, parse_nochar>
  {
  };
  struct parse_call : pegtl::seq<pegtl::string<'c', 'a', 'l', 'l'>, parse_nochar>
  {
  };
  struct parse_print : pegtl::seq<pegtl::string<'p', 'r', 'i', 'n', 't'>, parse_nochar>
  {
  };
  struct parse_input : pegtl::seq<pegtl::string<'i', 'n', 'p', 'u', 't'>, parse_nochar>
  {
  };
  struct parse_allocate : pegtl::seq<pegtl::string<'a', 'l', 'l', 'o', 'c', 'a', 't', 'e'>, parse_nochar>
  {
  };
  struct parse_tuple_error : pegtl::seq<pegtl::string<'t', 'u', 'p', 'l', 'e', '-', 'e', 'r', 'r', 'o', 'r'>, parse_nochar>
  {
  };
  struct parse_tensor_error : pegtl::seq<pegtl::string<'t', 'e', 'n', 's', 'o', 'r', '-', 'e', 'r', 'r', 'o', 'r'>, parse_nochar>
  {
  };
  struct parse_define : pegtl::seq<pegtl::string<'d', 'e', 'f', 'i', 'n', 'e'>, parse_nochar>
  {
  };
  struct parse_store : pegtl::seq<pegtl::string<'s', 't', 'o', 'r', 'e'>, parse_nochar>
  {
  };
  struct parse_load : pegtl::seq<pegtl::string<'l', 'o', 'a', 'd'>, parse_nochar>
  {
  };
  struct parse_br : pegtl::seq<pegtl::string<'b', 'r'>, parse_nochar>
  {
  };
  struct parse_comma : pegtl::seq<pegtl::one<','>, parse_nochar>
  {
  };

  std::string remove_perc(std::string str)
  {
    return str.substr(1);
  }
  std::string add_perc(std::string str)
  {
    return '%' + str;
  }
  /*
   * Templates
   */
  template <typename Rule>
  struct action : pegtl::nothing<Rule>
  {
  };

} // namespace L3