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

namespace IR
{
  struct parse_arrow : pegtl::string<'<', '-'>
  {
  };

  // parse t
  struct parse_num_t : parse_number
  {
  };
  struct parse_variable_t : parse_variable
  {
  };
  struct parse_t : pegtl::sor<parse_num_t, parse_variable_t>
  {
  };
  // parse s
  // s value variants
  struct parse_s_num_val : parse_number
  {
  };
  struct parse_s_fname_val : pegtl::seq<pegtl::one<'@'>, parse_name>
  {
  };
  struct parse_s_var : parse_variable
  {
  };
  struct parse_s : pegtl::sor<parse_s_fname_val, parse_s_num_val, parse_s_var>
  {
  };
  // parse w
  struct parse_var_w : parse_variable
  {
  };
  struct parse_w : pegtl::sor<parse_var_w>
  {
  };
  // parse u
  struct parse_w_u : parse_w
  {
  };
  struct parse_l_u : parse_s_fname_val
  {
  };
  struct parse_u : pegtl::sor<parse_w_u, parse_l_u>
  {
  };

  /*
  templates
  */
  // parse t actions
  template <>
  struct action<parse_num_t>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_t.push_back(t{N{std::stoll(in.string())}});
    }
  };
  template <>
  struct action<parse_variable_t>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_t.push_back(t{var{remove_perc(in.string())}});
    }
  };
  // parse_s actions
  template <>
  struct action<parse_s_num_val>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items.push_back(s{N{std::stoll(in.string())}});
    }
  };
  template <>
  struct action<parse_s_fname_val>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items.push_back(s{l{in.string()}});
    }
  };
  template <>
  struct action<parse_s_var>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items.push_back(s{var{remove_perc(in.string())}});
    }
  };
  // parse u actions
  template <>
  struct action<parse_l_u>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_u.push_back(u{l{in.string()}});
    }
  };
  template <>
  struct action<parse_w_u>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_items_u.push_back(L3::u{acc.parsed_vars.back()});
      acc.parsed_vars.pop_back();
    }
  };
  template <>
  struct action<parse_var_w>
  {
    template <typename Input>
    static void apply(const Input &in, GlobalAccumulator &acc)
    {
      acc.parsed_vars.push_back(L3::var{remove_perc(in.string())});
    }
  };

} // namespace IR