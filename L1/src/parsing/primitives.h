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

/*
 * Separators.
 */
struct parse_spaces : pegtl::star<pegtl::sor<pegtl::one<' '>,pegtl::one<'\t'>>> {};
struct parse_seps : pegtl::star<pegtl::seq<parse_spaces,pegtl::eol>> {};


/*
* Templates
*/
template <typename Rule>
struct action : pegtl::nothing<Rule> {
};
}