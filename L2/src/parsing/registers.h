#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include "primitives.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
struct parse_arrow : pegtl::string<'<', '-'> {};

// a
struct parse_rdi : pegtl::string<'r', 'd', 'i'> {};
struct parse_rsi : pegtl::string<'r', 's', 'i'> {};
struct parse_rdx : pegtl::string<'r', 'd', 'x'> {};
struct parse_rcx : pegtl::string<'r', 'c', 'x'> {};
struct parse_r8 : pegtl::string<'r', '8'> {};
struct parse_r9 : pegtl::string<'r', '9'> {};
// w
struct parse_rax : pegtl::string<'r', 'a', 'x'> {};
struct parse_rbx : pegtl::string<'r', 'b', 'x'> {};
struct parse_rbp : pegtl::string<'r', 'b', 'p'> {};
struct parse_r10 : pegtl::string<'r', '1', '0'> {};
struct parse_r11 : pegtl::string<'r', '1', '1'> {};
struct parse_r12 : pegtl::string<'r', '1', '2'> {};
struct parse_r13 : pegtl::string<'r', '1', '3'> {};
struct parse_r14 : pegtl::string<'r', '1', '4'> {};
struct parse_r15 : pegtl::string<'r', '1', '5'> {};
// x 
struct parse_rsp : pegtl::string<'r', 's', 'p'> {};
// a
struct parse_register_rdi : pegtl::seq<parse_rdi, parse_nochar> {};
struct parse_register_rsi : pegtl::seq<parse_rsi, parse_nochar> {};
struct parse_register_rdx : pegtl::seq<parse_rdx, parse_nochar> {};
struct parse_register_sx : pegtl::seq<parse_rcx, parse_nochar> {};
struct parse_register_r8 : pegtl::seq<parse_r8, parse_nochar> {};
struct parse_register_r9 : pegtl::seq<parse_r9, parse_nochar> {};
// w
struct parse_register_rax : pegtl::seq<parse_rax, parse_nochar> {};
struct parse_register_rbx : pegtl::seq<parse_rbx, parse_nochar> {};
struct parse_register_rbp : pegtl::seq<parse_rbp, parse_nochar> {};
struct parse_register_r10 : pegtl::seq<parse_r10, parse_nochar> {};
struct parse_register_r11 : pegtl::seq<parse_r11, parse_nochar> {};
struct parse_register_r12 : pegtl::seq<parse_r12, parse_nochar> {};
struct parse_register_r13 : pegtl::seq<parse_r13, parse_nochar> {};
struct parse_register_r14 : pegtl::seq<parse_r14, parse_nochar> {};
struct parse_register_r15 : pegtl::seq<parse_r15, parse_nochar> {};
// a
struct parse_register_rsp : pegtl::seq<parse_rsp, parse_nochar> {};
// parse all registers
struct parse_register : pegtl::sor<parse_register_rdi, parse_register_rsi, parse_register_rdx, 
parse_register_sx, parse_register_r8, parse_register_r9, parse_register_rax, parse_register_rbx, 
parse_register_rbp, parse_register_r10, parse_register_r11, parse_register_r12, parse_register_r13, parse_register_r14,
parse_register_r15, parse_register_rsp> {};
/*
templates
*/
// a 
template <>
struct action<parse_register_rdi> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rdi});
  }
};
template <>
struct action<parse_register_rsi> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rsi});
  }
};
template <>
struct action<parse_register_rdx> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rdx});
  }
};
template <>
struct action<parse_register_sx> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rcx});
  }
};
template <>
struct action<parse_register_r8> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r8});
  }
};
template <>
struct action<parse_register_r9> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r9});
  }
};

// w
template <>
struct action<parse_register_rax> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rax});
  }
};
template <>
struct action<parse_register_rbx> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rbx});
  }
};
template <>
struct action<parse_register_rbp> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rbp});
  }
};
template <>
struct action<parse_register_r10> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r10});
  }
};
template <>
struct action<parse_register_r11> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r11});
  }
};
template <>
struct action<parse_register_r12> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r12});
  }
};
template <>
struct action<parse_register_r13> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r13});
  }
};
template <>
struct action<parse_register_r14> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r14});
  }
};
template <>
struct action<parse_register_r15> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::r15});
  }
};

template <>
struct action<parse_register_rsp> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    parsed_items.push_back(s{r::rsp});
  }
};

} // namespace L1