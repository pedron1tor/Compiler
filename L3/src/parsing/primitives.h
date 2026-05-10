#pragma once

#include <L3.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L3 {
/*
tokens parsed
*/
struct GlobalAccumulator {
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
struct parse_variable : pegtl::seq<pegtl::one<'%'>, parse_name> {
};
struct parse_label : pegtl::seq<pegtl::one<':'>, parse_name> {
};
struct parse_comments : pegtl::seq<pegtl::string<'/', '/'>, pegtl::until<pegtl::eolf>> {
};
/*
 * Separators.
 */
struct parse_spaces : pegtl::star<pegtl::sor<pegtl::one<' '>, pegtl::one<'\t'>>> {
};
struct parse_seps : pegtl::star<pegtl::sor<pegtl::space, parse_comments>> {
};
struct parse_nochar : pegtl::not_at<pegtl::alnum> {
};
/*
aop
sop
cmp
*/
struct parse_plus_assign : pegtl::string<'+', '='> {
};
struct parse_minus_assign : pegtl::string<'-', '='> {
};
struct parse_mult_assign : pegtl::string<'*', '='> {
};
struct parse_and_assign : pegtl::string<'&', '='> {
};

struct parse_left_shift_assign : pegtl::string<'<', '<', '='> {
};
struct parse_right_shift_assign : pegtl::string<'>', '>', '='> {
};
struct parse_lt : pegtl::string<'<'> {
};
struct parse_gt : pegtl::string<'>'> {
};
struct parse_lte : pegtl::string<'<', '='> {
};
struct parse_gte : pegtl::string<'>', '='> {
};
struct parse_eq : pegtl::string<'='> {
};
struct parse_aop_assign : pegtl::sor<parse_plus_assign, parse_minus_assign, parse_mult_assign,
                                     parse_and_assign> {
};
struct parse_plus : pegtl::seq<pegtl::one<'+'>> {
};
struct parse_minus : pegtl::seq<pegtl::one<'-'>> {
};
struct parse_mult : pegtl::seq<pegtl::one<'*'>> {
};
struct parse_and : pegtl::seq<pegtl::one<'&'>> {
};
struct parse_rshift : pegtl::seq<pegtl::string<'>', '>'>> {
};
struct parse_lshift : pegtl::seq<pegtl::string<'<', '<'>> {
};
struct parse_aop : pegtl::sor<parse_plus, parse_minus, parse_mult, parse_and, parse_rshift, parse_lshift> {
};
struct parse_cmp : pegtl::sor<parse_lte, parse_lt, parse_eq, parse_gte, parse_gt> {
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
struct parse_return : pegtl::seq<pegtl::string<'r', 'e', 't', 'u', 'r', 'n'>, parse_nochar> {
};
struct parse_call : pegtl::seq<pegtl::string<'c', 'a', 'l', 'l'>, parse_nochar> {
};
struct parse_print : pegtl::seq<pegtl::string<'p', 'r', 'i', 'n', 't'>, parse_nochar> {
};
struct parse_input : pegtl::seq<pegtl::string<'i', 'n', 'p', 'u', 't'>, parse_nochar> {
};
struct parse_allocate : pegtl::seq<pegtl::string<'a', 'l', 'l', 'o', 'c', 'a', 't', 'e'>, parse_nochar> {
};
struct parse_tuple_error : pegtl::seq<pegtl::string<'t', 'u', 'p', 'l', 'e', '-', 'e', 'r', 'r', 'o', 'r'>, parse_nochar> {
};
struct parse_tensor_error : pegtl::seq<pegtl::string<'t', 'e', 'n', 's', 'o', 'r', '-', 'e', 'r', 'r', 'o', 'r'>, parse_nochar> {
};
struct parse_define : pegtl::seq<pegtl::string<'d', 'e', 'f', 'i', 'n', 'e'>, parse_nochar> {
};
struct parse_store : pegtl::seq<pegtl::string<'s', 't', 'o', 'r', 'e'>, parse_nochar> {
};
struct parse_load : pegtl::seq<pegtl::string<'l', 'o', 'a', 'd'>, parse_nochar> {
};
struct parse_br : pegtl::seq<pegtl::string<'b', 'r'>, parse_nochar> {
};
struct parse_comma : pegtl::one<','> {
};

inline std::string remove_perc(std::string str) {
  return str.substr(1);
}
inline std::string add_perc(std::string str) {
  return '%' + str;
}
// to/translation functions
inline s t_to_s(t &t_item) {
  return std::visit([](auto &v) -> s { return s{v}; }, t_item);
}
inline s w_to_s(w &w_item) {
  return std::visit([](auto &v) -> s { return s{v}; }, w_item);
}
inline Op toOp(aop operation) {
  switch (operation) {
  case aop::add:
    return Op::Add;
  case aop::sub:
    return Op::Sub;
  case aop::mul:
    return Op::Mul;
  case aop::and_:
    return Op::And;
  case aop::lshift:
    return Op::Shl;
  case aop::rshift:
    return Op::Shr;
  default:
    __builtin_unreachable();
  }
}

inline Op toCmpOp(cmp operation) {
  switch (operation) {
  case cmp::lt:
    return Op::Lt;
  case cmp::gt:
    return Op::Gt;
  case cmp::le:
    return Op::Le;
  case cmp::gte:
    return Op::Ge;
  case cmp::eq:
    return Op::Eq;
  default:
    __builtin_unreachable();
  }
}

inline t s_to_t(const s &sv) {
  return std::visit([](auto &x) -> t {
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, r>)
      return t{x};
    else if constexpr (std::is_same_v<X, N>)
      return t{x};
    else if constexpr (std::is_same_v<X, var>)
      return t{x};
    else {
      __builtin_unreachable();
    }
  },
                    sv);
}

inline w s_to_w(const s &sv) {
  return std::visit([](auto &x) -> w {
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, r>)
      return w{x};
    else if constexpr (std::is_same_v<X, var>)
      return w{x};
    else {
      __builtin_unreachable();
    }
  },
                    sv);
}

inline aop op_to_aop(Op op) {
  switch (op) {
  case Op::Add:
    return aop::add;
  case Op::Sub:
    return aop::sub;
  case Op::Mul:
    return aop::mul;
  case Op::And:
    return aop::and_;
  case Op::Shl:
    return aop::lshift;
  case Op::Shr:
    return aop::rshift;
  default:
    __builtin_unreachable();
  }
}
inline cmp op_to_cmp(Op cmp_op) {
  switch (cmp_op) {
  case Op::Lt:
    return cmp::lt;
  case Op::Le:
    return cmp::le;
  case Op::Eq:
    return cmp::eq;
  case Op::Gt:
    return cmp::lt;
  case Op::Ge:
    return cmp::le;
  default:
    __builtin_unreachable();
  }
}
inline std::string
r_to_str(r reg) {
  switch (reg) {
  case r::rax:
    return "rax";
  case r::rbx:
    return "rbx";
  case r::rbp:
    return "rbp";
  case r::r10:
    return "r10";
  case r::r11:
    return "r11";
  case r::r12:
    return "r12";
  case r::r13:
    return "r13";
  case r::r14:
    return "r14";
  case r::r15:
    return "r15";
  case r::rdi:
    return "rdi";
  case r::rsi:
    return "rsi";
  case r::rdx:
    return "rdx";
  case r::r8:
    return "r8";
  case r::r9:
    return "r9";
  case r::rcx:
    return "rcx";
  case r::rsp:
    return "rsp";
  }
  __builtin_unreachable();
};
inline std::string t_to_str(t val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return std::to_string(v.value); },
                    val);
}
inline std::string s_to_str(s val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, N>)
      return std::to_string(v.value);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return v.name; },
                    val);
}
inline std::string w_to_str(w val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, var>)
      return add_perc(v.name);
    else
      return v.name; },
                    val);
}
inline std::string aop_to_str(aop op) {
  switch (op) {
  case aop::add:
    return "+";
  case aop::sub:
    return "-";
  case aop::mul:
    return "*";
  case aop::and_:
    return "&";
  case aop::lshift:
    return "<<";
  case aop::rshift:
    return ">>";
  }
  __builtin_unreachable();
}
inline std::string aop_to_str_l2(aop op) {
  switch (op) {
  case aop::add:
    return "+=";
  case aop::sub:
    return "-=";
  case aop::mul:
    return "*=";
  case aop::and_:
    return "&=";
  case aop::lshift:
    return "<<=";
  case aop::rshift:
    return ">>=";
  }
  __builtin_unreachable();
}
inline std::string sop_to_str(sop op) {
  switch (op) {
  case sop::lshift:
    return "<<=";
  case sop::rshift:
    return ">>=";
  }
  __builtin_unreachable();
}
inline std::string cmp_to_str(cmp op) {
  switch (op) {
  case cmp::lt:
    return "<";
  case cmp::gt:
    return ">";
  case cmp::le:
    return "<=";
  case cmp::eq:
    return "=";
  case cmp::gte:
    return ">=";
  }
  __builtin_unreachable();
}

inline std::string callee_to_str(callee_type &callee) {
  return std::visit([](auto &c) -> std::string {
      using C = std::decay_t<decltype(c)>;
      if constexpr (std::is_same_v<C, CallPrint>) return "print";
      else if constexpr (std::is_same_v<C, CallInput>) return "input";
      else if constexpr (std::is_same_v<C, CallAllocate>) return "allocate";
      else if constexpr (std::is_same_v<C, CallTupleError>) return "tuple-error";
      else if constexpr (std::is_same_v<C, CallTensorError>) return "tensor-error";
      else return std::visit([](auto &v) -> std::string {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, r>) return r_to_str(v);
        else if constexpr (std::is_same_v<V, var>) return add_perc(v.name);
        else return v.name;
      }, c); }, callee);
}
inline std::string call_args_to_str(std::vector<t> &args) {
  std::string result = "(";
  bool first = true;
  for (auto &arg : args) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }

    result += t_to_str(arg);
  }
  return result + ")";
}

template <typename V>
bool is_var(const V &val, const var &v) {
  return std::visit([&](const auto &x) -> bool {
    // if the value in the instruction is a variable, compare against variable of interest
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, var>)
      return x == v;
    else
      return false;
  },
                    val);
};
template <typename V>
bool is_var_type(const V &val) {
  return std::visit([&](const auto &x) -> bool {
    // if the value in the instruction is a variable, compare against variable of interest
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, var>)
      return true else return false;
  },
                    val);
};

/*
 * Templates
 */
template <typename Rule>
struct action : pegtl::nothing<Rule> {
};

} // namespace L3