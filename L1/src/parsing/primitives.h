#pragma once

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
#include <L1.h>
#include <parser.h>


namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L1 {
/*
tokens parsed
*/
std::vector<s> parsed_items;
std::vector<t> parsed_items_t;
std::vector<aop> parsed_ops;
std::vector<N> parsed_offset;

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
struct parse_label : pegtl::seq<pegtl::one<':'>,parse_name> {};
struct parse_comments : pegtl::seq<pegtl::string<'/', '/'>, pegtl::until<pegtl::eolf>> {};
/*
 * Separators.
 */
struct parse_spaces : pegtl::star<pegtl::sor<pegtl::one<' '>,pegtl::one<'\t'>>> {};
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
/*
* Templates
*/
template <typename Rule>
struct action : pegtl::nothing<Rule> {
};
}