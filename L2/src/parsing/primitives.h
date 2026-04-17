#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
/*
tokens parsed
*/
struct GlobalAccumulator {
  p program;
  std::vector<s> parsed_items;
  std::vector<t> parsed_items_t;
  std::vector<aop> parsed_ops;
  std::vector<sop> parsed_sops;
  std::vector<cmp> parsed_cmps;
  std::vector<N> parsed_offset;
  std::vector<label> parsed_labels;
  std::vector<u> parsed_items_u;
};

struct parse_number : pegtl::seq<pegtl::sor<
                          pegtl::seq<
                              pegtl::opt<pegtl::sor<pegtl::one<'+'>, pegtl::one<'-'>>>,
                              pegtl::range<'1', '9'>,
                              pegtl::star<pegtl::digit>>,
                          pegtl::one<'0'>>> {
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
                                pegtl::digit>>> {
};
struct parse_variable : pegtl::seq<pegtl::one<'%'>, parse_name> {};
struct parse_label : pegtl::seq<pegtl::one<':'>, parse_name> {};
struct parse_comments : pegtl::seq<pegtl::string<'/', '/'>, pegtl::until<pegtl::eolf>> {};
/*
 * Separators.
 */
struct parse_spaces : pegtl::star<pegtl::sor<pegtl::one<' '>, pegtl::one<'\t'>>> {};
struct parse_seps : pegtl::star<pegtl::sor<pegtl::space, parse_comments>> {};
struct parse_nochar : pegtl::not_at<pegtl::alnum> {};
/*
aop
sop
cmp
*/
struct parse_plus_assign : pegtl::string<'+', '='> {};
struct parse_minus_assign : pegtl::string<'-', '='> {};
struct parse_mult_assign : pegtl::string<'*', '='> {};
struct parse_and_assign : pegtl::string<'&', '='> {};
struct parse_left_shift_assign : pegtl::string<'<', '<', '='> {};
struct parse_right_shift_assign : pegtl::string<'>', '>', '='> {};
struct parse_lt : pegtl::string<'<'> {};
struct parse_lte : pegtl::string<'<', '='> {};
struct parse_eq : pegtl::string<'='> {};
struct parse_aop : pegtl::sor<parse_plus_assign, parse_minus_assign, parse_mult_assign,
                              parse_and_assign> {};
struct parse_sop : pegtl::sor<parse_left_shift_assign, parse_right_shift_assign> {};
struct parse_cmp : pegtl::sor<parse_lte, parse_lt, parse_eq> {};
/*
keywords
return
mem
goto
cjump
call
print
input
allocate
tuple-error
tensor-error
*/
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
// parse t
struct parse_num_t : parse_number {};
struct parse_x_t : parse_register {};
struct parse_variable_t : parse_variable {};
struct parse_t : pegtl::sor<parse_x_t, parse_num_t, parse_variable_t> {};
// parse s
// s value variants
struct parse_s_num_val : parse_number {};
struct parse_s_label_val : parse_label {};
struct parse_s_fname_val : pegtl::seq<pegtl::one<'@'>, parse_name> {};
struct parse_s : pegtl::sor<parse_s_label_val, parse_s_fname_val, parse_register, parse_s_num_val> {};
// parse u
struct parse_w_u : parse_register {};
struct parse_l_u : parse_s_fname_val {};
struct parse_u : pegtl::sor<parse_w_u, parse_l_u> {};
/*
 * Templates
 */
template <typename Rule>
struct action : pegtl::nothing<Rule> {
};
// parse t actions
template <>
struct action<parse_num_t> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_t.push_back(t{N{std::stoll(in.string())}});
  }
};
template <>
struct action<parse_x_t> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_t.push_back(t{str_to_r(in.string())});
  }
};
template <>
struct action<parse_variable_t> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_t.push_back(t{in.string()[1:]});
  }
};
// parse_s actions
template <>
struct action<parse_s_num_val> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items.push_back(s{N{std::stoll(in.string())}});
  }
};
template <>
struct action<parse_s_label_val> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items.push_back(s{label{in.string()}});
  }
};
template <>
struct action<parse_s_fname_val> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items.push_back(s{l{in.string()}});
  }
};
// parse u actions
template <>
struct action<parse_l_u> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_u.push_back(u{l{in.string()}});
  }
};
template <>
struct action<parse_w_u> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_u.push_back(u{str_to_r(in.string())});
  }
};
} // namespace L2