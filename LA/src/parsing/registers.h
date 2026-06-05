#pragma once

#include <IR.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
#include <unordered_map>

#include "primitives.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace LA {
struct parse_arrow : pegtl::string<'<', '-'> {
};

// parse t
struct parse_num_t : parse_number {
};
struct parse_variable_t : parse_variable {
};
struct parse_t : pegtl::sor<parse_num_t, parse_variable_t> {
};
/*
templates
*/
// parse t actions
template <>
struct action<parse_num_t> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_t.push_back(LA::t{L3::N{std::stoll(in.string())}});
  }
};
template <>
struct action<parse_variable_t> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_t.push_back(LA::t{in.string()});
  }
};

} // namespace LA