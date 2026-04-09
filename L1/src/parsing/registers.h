#pragma once

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
#include <L1.h>
#include <parser.h>

#include "primitives.h"


namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L1 {
    struct parse_arrow : pegtl::string<'<', '-'> {};
struct parse_rdi : pegtl::string<'r', 'd', 'i'> {};
struct parse_rax : pegtl::string<'r', 'a', 'x'> {};

struct parse_register_rdi : parse_rdi {};
struct parse_register_rax : parse_rax {};
struct parse_register : pegtl::sor<parse_register_rdi, parse_register_rax> {};

/*
templates
*/
template <>
struct action<parse_register_rdi> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
parsed_items.push_back(s{r::rdi});
  }
};

template <>
struct action<parse_register_rax> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rax});
  }
};
}