#include <algorithm>
#include <assert.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sched.h>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

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
/*
 * Grammar rules definitions from now on.
 */

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

struct parse_label : pegtl::seq<
                         pegtl::one<':'>,
                         parse_name> {
};

struct parse_function_name : pegtl::seq<
                                      pegtl::one<'@'>,
                                      parse_name> {
};

/*
 * Separators.
 */
struct parse_spaces : pegtl::star<
                          pegtl::sor<
                              pegtl::one<' '>,
                              pegtl::one<'\t'>>> {
};

struct parse_seps : pegtl::star<
                        pegtl::seq<
                            parse_spaces,
                            pegtl::eol>> {
};

struct parse_arrow : pegtl::string<'<', '-'> {};
struct parse_rdi : pegtl::string<'r', 'd', 'i'> {};
struct parse_rax : pegtl::string<'r', 'a', 'x'> {};

struct parse_register_rdi : parse_rdi {};

struct parse_register_rax : parse_rax {};

struct parse_register : pegtl::sor<
                           parse_register_rdi,
                           parse_register_rax> {};

struct parse_nochar : pegtl::not_at<pegtl::alnum> {
};
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

struct parse_call_print : pegtl::seq<parse_call, parse_spaces, parse_print, parse_spaces, parse_number> {
};

struct parse_call_input : pegtl::seq<parse_call, parse_spaces, parse_input, parse_spaces, parse_number> {
};

struct parse_call_allocate : pegtl::seq<parse_call, parse_spaces, parse_allocate, parse_spaces, parse_number> {
};

struct parse_call_tuple_error : pegtl::seq<parse_call, parse_spaces, parse_tuple_error, parse_spaces, parse_number> {
};

struct parse_call_tensor_error : pegtl::seq<parse_call, parse_spaces, parse_tensor_error, parse_spaces, parse_number> {
};

struct parse_label_inst : parse_label {
};
struct parse_assignment:
    pegtl::seq<
      parse_register,
      parse_spaces,
      parse_arrow,
      parse_spaces,
      parse_register
    > {};
struct parse_instructions : pegtl::seq<parse_seps,
                                       pegtl::plus<pegtl::seq<parse_spaces, pegtl::sor<parse_return, parse_call_print, parse_call_input, parse_call_allocate, parse_call_tuple_error, parse_call_tensor_error, parse_label_inst>, parse_spaces, parse_seps>>> {
};

struct parse_argument_number : parse_number {
};

struct parse_local_number : parse_number {
};

struct parse_function_rule : pegtl::seq<parse_seps,
                                        pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                        parse_seps,
                                        parse_function_name,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, parse_argument_number>,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, parse_local_number>,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, parse_instructions>,
                                        parse_seps,
                                        parse_seps,
                                        pegtl::seq<parse_spaces, pegtl::one<')'>>,
                                        parse_seps> {
};
struct parse_functions_rule : pegtl::seq<
                                  pegtl::plus<parse_function_rule>> {
};
struct parse_entry_point_rule : pegtl::seq<
                                    parse_seps,
                                    pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                    parse_seps,
                                    parse_function_name,
                                    parse_seps,
                                    parse_functions_rule,
                                    pegtl::seq<parse_spaces, pegtl::one<')'>>,
                                    parse_seps> {
};

struct grammar : pegtl::must<
                     parse_entry_point_rule> {
};

/*
 * Actions attached to grammar rules.
 */
template <typename Rule>
struct action : pegtl::nothing<Rule> {
};

template <>
struct action<parse_function_name> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    if (program.entry_point.name.empty()) {
      program.entry_point.name = in.string();
    } else {
      f newF;
      newF.name.name = in.string();
      program.functions.push_back(newF);
    }
  }
};
template <>
struct action<parse_argument_number> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto &currentF = program.functions.back();
    currentF.num_args = std::stoll(in.string());
  }
};
template <>
struct action<parse_local_number> {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto &currentF = program.functions.back();
    currentF.num_locals = std::stoll(in.string());
  }
};

template <typename Rule, typename Inst>
struct action_push {
  template <typename Input>
  static void apply(const Input &in, p &program) {
    auto &currentF = program.functions.back();
    currentF.instructions.push_back(i{Inst{}});
  }
};

template <>
struct action<parse_return> : action_push<parse_return, Return> {};
template <>
struct action<parse_call_print> : action_push<parse_call_print, CallPrint> {};
template <>
struct action<parse_call_input> : action_push<parse_call_input, CallInput> {};
template <>
struct action<parse_call_allocate> : action_push<parse_call_allocate, CallAllocate> {};
template <>
struct action<parse_call_tuple_error> : action_push<parse_call_tuple_error, CallTupleError> {};
template <>
struct action<parse_call_tensor_error> : action_push<parse_call_tensor_error, CallTensorError> {};

template <> struct action<parse_label_inst> {
    template <typename Input>
    static void apply(const Input &in, p &program) {
        program.functions.back().instructions.push_back(i{LabelInst{label{in.string()}}});
    }
};
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

template <> struct action<parse_assignment> {
    template <typename Input>
    static void apply(const Input &in, p &program) {
        auto src = parsed_items.back(); parsed_items.pop_back();
        auto dst = parsed_items.back(); parsed_items.pop_back();
        auto newI = Assign{ std::get<r>(dst), src };
        program.functions.back().instructions.push_back(i{newI});
    }
};

p parse_file(char *filename) {
  /*
   * Check the grammar for some possible issues.
   */
  if (pegtl::analyze<grammar>() != 0) {
    std::cerr << "There are problems with the grammar" << std::endl;
    exit(1);
  }

  /*
   * Parse.
   */
  file_input<> fileInput(filename);
  p program_ast;
  parse<grammar, action>(fileInput, program_ast);
  std::cerr << "=== AST ===\n";
  std::cerr << "Entry point: " << program_ast.entry_point.name << "\n";
  std::cerr << "Functions: " << program_ast.functions.size() << "\n";
  for (auto &fn : program_ast.functions) {
    std::cerr << "  Function: " << fn.name.name << "\n";
    std::cerr << "    args: " << fn.num_args << "\n";
    std::cerr << "    locals: " << fn.num_locals << "\n";
    std::cerr << "    instructions: " << fn.instructions.size() << "\n";
  }
  return program_ast;
}
} // namespace L1