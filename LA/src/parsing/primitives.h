#pragma once

#include <LA.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace LA {
/*
tokens parsed
*/
struct GlobalAccumulator {
  pLA program;
  std::vector<LA::t> parsed_items_t;
  std::vector<IR::op> parsed_ops;
  std::vector<L3::label> parsed_labels;
  std::vector<LA::t> parsed_call_args;
  std::vector<IR::type> parsed_types;
  std::vector<LA::t> parsed_indices;
  std::vector<std::string> parsed_names;
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
struct parse_variable : parse_name {
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
,
void
int64
code
tuple
new
length
Array (constructor)
Tuple (constructor)
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
struct parse_store : pegtl::seq<pegtl::string<'s', 't', 'o', 'r', 'e'>, parse_nochar> {
};
struct parse_load : pegtl::seq<pegtl::string<'l', 'o', 'a', 'd'>, parse_nochar> {
};
struct parse_br : pegtl::seq<pegtl::string<'b', 'r'>, parse_nochar> {
};
struct parse_comma : pegtl::one<','> {
};

struct parse_void : pegtl::seq<pegtl::string<'v', 'o', 'i', 'd'>, parse_nochar> {};
struct parse_int64 : pegtl::seq<pegtl::string<'i', 'n', 't', '6', '4'>, parse_nochar> {};
struct parse_code : pegtl::seq<pegtl::string<'c', 'o', 'd', 'e'>, parse_nochar> {};
struct parse_tuple_type : pegtl::seq<
                              pegtl::string<'t', 'u', 'p', 'l', 'e'>, // ignore tuple-type
                              pegtl::not_at<pegtl::sor<pegtl::alnum, pegtl::one<'-'>>>> {};

struct parse_new : pegtl::seq<pegtl::string<'n', 'e', 'w'>, parse_nochar> {};
struct parse_length : pegtl::seq<pegtl::string<'l', 'e', 'n', 'g', 't', 'h'>, parse_nochar> {};
struct parse_array_constructor : pegtl::seq<pegtl::string<'A', 'r', 'r', 'a', 'y'>, parse_nochar> {};
struct parse_tuple_constructor : pegtl::seq<pegtl::string<'T', 'u', 'p', 'l', 'e'>, parse_nochar> {};

struct parse_bracket_pair : pegtl::string<'[', ']'> {};
struct parse_type_int64 : pegtl::seq<parse_int64, pegtl::star<parse_bracket_pair>> {};
struct parse_type_tuple : parse_tuple_type {};
struct parse_type_code : parse_code {};
// parse_T and parse_type
struct parse_type : pegtl::sor<parse_type_int64, parse_type_tuple, parse_type_code> {};
struct parse_T_void : parse_void {};
struct parse_T : pegtl::sor<parse_type, parse_T_void> {};

inline std::string remove_perc(std::string str) {
  return str.substr(1);
}
inline std::string add_perc(std::string str) {
  return '%' + str;
}
// to/translation functions
inline L3::s t_to_s(L3::t &t_item) {
  return std::visit([](auto &v) -> L3::s { return L3::s{v}; }, t_item);
}
inline L3::s w_to_s(L3::w &w_item) {
  return std::visit([](auto &v) -> L3::s { return L3::s{v}; }, w_item);
}
inline L3::Op toOp(L3::aop operation) {
  switch (operation) {
  case L3::aop::add:
    return L3::Op::Add;
  case L3::aop::sub:
    return L3::Op::Sub;
  case L3::aop::mul:
    return L3::Op::Mul;
  case L3::aop::and_:
    return L3::Op::And;
  case L3::aop::lshift:
    return L3::Op::Shl;
  case L3::aop::rshift:
    return L3::Op::Shr;
  default:
    __builtin_unreachable();
  }
}

inline L3::Op toCmpOp(L3::cmp operation) {
  switch (operation) {
  case L3::cmp::lt:
    return L3::Op::Lt;
  case L3::cmp::gt:
    return L3::Op::Gt;
  case L3::cmp::le:
    return L3::Op::Le;
  case L3::cmp::gte:
    return L3::Op::Ge;
  case L3::cmp::eq:
    return L3::Op::Eq;
  default:
    __builtin_unreachable();
  }
}

inline L3::t s_to_t(const L3::s &sv) {
  return std::visit([](auto &x) -> L3::t {
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, L3::r>)
      return L3::t{x};
    else if constexpr (std::is_same_v<X, L3::N>)
      return L3::t{x};
    else if constexpr (std::is_same_v<X, L3::var>)
      return L3::t{x};
    else {
      __builtin_unreachable();
    } },
                    sv);
}

inline L3::w s_to_w(const L3::s &sv) {
  return std::visit([](auto &x) -> L3::w {
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, L3::r>)
      return L3::w{x};
    else if constexpr (std::is_same_v<X, L3::var>)
      return L3::w{x};
    else {
      __builtin_unreachable();
    } },
                    sv);
}

inline L3::aop op_to_aop(L3::Op op) {
  switch (op) {
  case L3::Op::Add:
    return L3::aop::add;
  case L3::Op::Sub:
    return L3::aop::sub;
  case L3::Op::Mul:
    return L3::aop::mul;
  case L3::Op::And:
    return L3::aop::and_;
  case L3::Op::Shl:
    return L3::aop::lshift;
  case L3::Op::Shr:
    return L3::aop::rshift;
  default:
    __builtin_unreachable();
  }
}
inline L3::cmp op_to_cmp(L3::Op cmp_op) {
  switch (cmp_op) {
  case L3::Op::Lt:
    return L3::cmp::lt;
  case L3::Op::Le:
    return L3::cmp::le;
  case L3::Op::Eq:
    return L3::cmp::eq;
  case L3::Op::Gt:
    return L3::cmp::lt;
  case L3::Op::Ge:
    return L3::cmp::le;
  default:
    __builtin_unreachable();
  }
}
inline std::string
r_to_str(L3::r reg) {
  switch (reg) {
  case L3::r::rax:
    return "rax";
  case L3::r::rbx:
    return "rbx";
  case L3::r::rbp:
    return "rbp";
  case L3::r::r10:
    return "r10";
  case L3::r::r11:
    return "r11";
  case L3::r::r12:
    return "r12";
  case L3::r::r13:
    return "r13";
  case L3::r::r14:
    return "r14";
  case L3::r::r15:
    return "r15";
  case L3::r::rdi:
    return "rdi";
  case L3::r::rsi:
    return "rsi";
  case L3::r::rdx:
    return "rdx";
  case L3::r::r8:
    return "r8";
  case L3::r::r9:
    return "r9";
  case L3::r::rcx:
    return "rcx";
  case L3::r::rsp:
    return "rsp";
  }
  __builtin_unreachable();
};
inline std::string L3t_to_str(L3::t val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, L3::r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, L3::var>)
      return add_perc(v.name);
    else
      return std::to_string(v.value); },
                    val);
}
inline std::string t_to_str(t val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, std::string>)
      return v;
    else
      return std::to_string(v.value); },
                    val);
}
inline std::string s_to_str(L3::s val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, L3::r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, L3::N>)
      return std::to_string(v.value);
    else if constexpr (std::is_same_v<T, L3::var>)
      return add_perc(v.name);
    else
      return v.name; },
                    val);
}
inline std::string type_to_str(IR::type function_type) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, IR::t_void>)
      return "void ";
    else if constexpr (std::is_same_v<T, IR::t_tuple>)
      return "tuple ";
    else if constexpr (std::is_same_v<T, IR::t_int64>) {
      std::string repeated = "";
      for(int i = 0; i < v.dim; ++i) {
          repeated += "[]";
      };
      return "int64" + repeated + " ";
}
    else if constexpr (std::is_same_v<T, IR::t_code>)
      return "code ";
    else
      return "failure "; },
                    function_type);
};

inline std::string dims_to_str(std::vector<LA::t> dims) {
  std::string repeated = "";
  bool first = true;
  for (int i = 0; i < dims.size(); i++) {
    if (!first)
      repeated += ", ";
    first = false;
    repeated += t_to_str(dims[i]);
  };
  return repeated;
};
inline std::string L3dims_to_str(std::vector<L3::t> dims) {
  std::string repeated = "";
  bool first = true;
  for (int i = 0; i < dims.size(); i++) {
    if (!first)
      repeated += ", ";
    first = false;
    repeated += L3t_to_str(dims[i]);
  };
  return repeated;
};

inline std::string w_to_str(L3::w val) {
  return std::visit([](auto &v) -> std::string {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, L3::r>)
      return r_to_str(v);
    else if constexpr (std::is_same_v<T, L3::var>)
      return add_perc(v.name);
    else
      return v.name; },
                    val);
}
inline std::string aop_to_str(L3::aop op) {
  switch (op) {
  case L3::aop::add:
    return "+";
  case L3::aop::sub:
    return "-";
  case L3::aop::mul:
    return "*";
  case L3::aop::and_:
    return "&";
  case L3::aop::lshift:
    return "<<";
  case L3::aop::rshift:
    return ">>";
  }
  __builtin_unreachable();
}
inline std::string aop_to_str_l2(L3::aop op) {
  switch (op) {
  case L3::aop::add:
    return "+=";
  case L3::aop::sub:
    return "-=";
  case L3::aop::mul:
    return "*=";
  case L3::aop::and_:
    return "&=";
  case L3::aop::lshift:
    return "<<=";
  case L3::aop::rshift:
    return ">>=";
  }
  __builtin_unreachable();
}
inline std::string sop_to_str(L3::sop op) {
  switch (op) {
  case L3::sop::lshift:
    return "<<=";
  case L3::sop::rshift:
    return ">>=";
  }
  __builtin_unreachable();
}
inline std::string cmp_to_str(L3::cmp op) {
  switch (op) {
  case L3::cmp::lt:
    return "<";
  case L3::cmp::gt:
    return ">";
  case L3::cmp::le:
    return "<=";
  case L3::cmp::eq:
    return "=";
  case L3::cmp::gte:
    return ">=";
  }
  __builtin_unreachable();
}

inline std::string op_to_str(IR::op operation) {
  switch (operation) {
  case IR::op::add:
    return "+";
  case IR::op::sub:
    return "-";
  case IR::op::mul:
    return "*";
  case IR::op::and_:
    return "&";
  case IR::op::lshift:
    return "<<";
  case IR::op::rshift:
    return ">>";
  case IR::op::lt:
    return "<";
  case IR::op::gt:
    return ">";
  case IR::op::le:
    return "<=";
  case IR::op::gte:
    return ">=";
  case IR::op::eq:
    return "=";
  }
  __builtin_unreachable();
};
inline IR::op str_to_op(std::string operation) {
  if (operation == "+")
    return IR::op::add;
  else if (operation == "-")
    return IR::op::sub;
  else if (operation == "*")
    return IR::op::mul;
  else if (operation == "&")
    return IR::op::and_;
  else if (operation == "<<")
    return IR::op::lshift;
  else if (operation == ">>")
    return IR::op::rshift;
  else if (operation == "<")
    return IR::op::lt;
  else if (operation == ">")
    return IR::op::gt;
  else if (operation == "<=")
    return IR::op::le;
  else if (operation == ">=")
    return IR::op::gte;
  else if (operation == "=")
    return IR::op::eq;
  __builtin_unreachable();
}
inline std::string L3indices_to_str(std::vector<L3::t> indices) {
  std::string repeated = "";
  for (int i = 0; i < indices.size(); i++) {
    repeated += "[";
    repeated += L3t_to_str(indices[i]);
    repeated += "]";
  };
  return repeated;
};
inline std::string indices_to_str(std::vector<t> indices) {
  std::string repeated = "";
  for (int i = 0; i < indices.size(); i++) {
    repeated += "[";
    repeated += t_to_str(indices[i]);
    repeated += "]";
  };
  return repeated;
};

inline std::string callee_to_str(L3::callee_type &callee) {
  return std::visit([](auto &c) -> std::string {
      using C = std::decay_t<decltype(c)>;
      if constexpr (std::is_same_v<C, L3::CallPrint>) return "print";
      else if constexpr (std::is_same_v<C, L3::CallInput>) return "input";
      else if constexpr (std::is_same_v<C, L3::CallAllocate>) return "allocate";
      else if constexpr (std::is_same_v<C, L3::CallTupleError>) return "tuple-error";
      else if constexpr (std::is_same_v<C, L3::CallTensorError>) return "tensor-error";
      else return std::visit([](auto &v) -> std::string {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, L3::r>) return r_to_str(v);
        else if constexpr (std::is_same_v<V, L3::var>) return add_perc(v.name);
        else return v.name;
      }, c); }, callee);
}
inline std::string call_args_to_str(std::vector<LA::t> &args) {
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
inline std::string L3call_args_to_str(std::vector<L3::t> &args) {
  std::string result = "(";
  bool first = true;
  for (auto &arg : args) {
    if (first) {
      first = false;
    } else {
      result += ", ";
    }

    result += L3t_to_str(arg);
  }
  return result + ")";
}

inline L3::i BinOp_to_l3op(IR::BinOp &b) {
  L3::w dest{b.dest};
  switch (b.operation) {
  case IR::op::add:
    return L3::AopL3{dest, b.left, L3::aop::add, b.right};
  case IR::op::sub:
    return L3::AopL3{dest, b.left, L3::aop::sub, b.right};
  case IR::op::mul:
    return L3::AopL3{dest, b.left, L3::aop::mul, b.right};
  case IR::op::and_:
    return L3::AopL3{dest, b.left, L3::aop::and_, b.right};
  case IR::op::lshift:
    return L3::AopL3{dest, b.left, L3::aop::lshift, b.right};
  case IR::op::rshift:
    return L3::AopL3{dest, b.left, L3::aop::rshift, b.right};
  case IR::op::lt:
    return L3::Compare{dest, b.left, L3::cmp::lt, b.right};
  case IR::op::gt:
    return L3::Compare{dest, b.left, L3::cmp::gt, b.right};
  case IR::op::le:
    return L3::Compare{dest, b.left, L3::cmp::le, b.right};
  case IR::op::gte:
    return L3::Compare{dest, b.left, L3::cmp::gte, b.right};
  default:
    return L3::Compare{dest, b.left, L3::cmp::eq, b.right};
  }
}

template <typename V>
bool is_var(const V &val, const L3::var &v) {
  return std::visit([&](const auto &x) -> bool {
    // if the value in the instruction is a variable, compare against variable of interest
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, L3::var>)
      return x == v;
    else
      return false; },
                    val);
};

/*
 * Templates
 */
template <typename Rule>
struct action : pegtl::nothing<Rule> {
};

template <>
struct action<parse_type_int64> {
  template <typename Input>
  static void apply(const Input &in, GlobalAccumulator &acc) {
    int len_brackets = in.string().size() - 5; // remove the int64 prefix
    int dim = (len_brackets) / 2;              // count the pairs of brackets
    acc.parsed_types.push_back(IR::type{IR::t_int64{dim}});
  }
};
template <>
struct action<parse_type_tuple> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    acc.parsed_types.push_back(IR::type{IR::t_tuple{}});
  }
};
template <>
struct action<parse_type_code> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    acc.parsed_types.push_back(IR::type{IR::t_code{}});
  }
};
template <>
struct action<parse_T_void> {
  template <typename Input>
  static void apply(const Input &, GlobalAccumulator &acc) {
    acc.parsed_types.push_back(IR::type{IR::t_void{}});
  }
};

} // namespace LA