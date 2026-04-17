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
// parse w
struct parse_r_w : parse_register {};
struct parse_var_w : parse_variable {};
struct parse_w : pegtl::sor<parse_r_w, parse_var_w> {};
/*
templates
*/
r str_to_r(const std::string &s) {
  static const std::unordered_map<std::string, r> reg_map = {
      {"rax", r::rax}, {"rbx", r::rbx}, {"rbp", r::rbp}, {"r10", r::r10}, {"r11", r::r11}, {"r12", r::r12}, {"r13", r::r13}, {"r14", r::r14}, {"r15", r::r15}, {"rdi", r::rdi}, {"rsi", r::rsi}, {"rdx", r::rdx}, {"r8", r::r8}, {"r9", r::r9}, {"rcx", r::rcx}, {"rsp", r::rsp}};

  auto it = reg_map.find(s);
  if (it != reg_map.end()) {
    return it->second;
  }
  std::cerr << "unknown register: " << s << std::endl;
  abort();
}
// parse registers using the map helper
template <>
struct action<parse_register> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items.push_back(s{str_to_r(in.string())});
  };
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
    acc.parsed_items_t.push_back(t{var{remove_perc(in.string())}});
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
template <>
struct action<parse_var_w> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_w.push_back(w{var{remove_perc(in.string())}});
  }
};
template <>
struct action<parse_r_w> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    acc.parsed_items_w.push_back(w{str_to_r(in.string())});
  }
};
} // namespace L2