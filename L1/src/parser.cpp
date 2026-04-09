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
#include <tao/pegtl/contrib/trace.hpp>

#include <L1.h>
#include <parser.h>

#include "parsing/instructions.h"
#include "parsing/registers.h"
#include "parsing/primitives.h"

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L1 {

/*
 * Grammar rules definitions from now on.
 */

struct parse_function_name : pegtl::seq<pegtl::one<'@'>,parse_name> {};


struct parse_argument_number : parse_number {};
struct parse_local_number : parse_number {};

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
                                        parse_seps> {};
struct parse_functions_rule : pegtl::seq<
                                  pegtl::plus<parse_function_rule>> {};
struct parse_entry_point_rule : pegtl::seq<
                                    parse_seps,
                                    pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                    parse_seps,
                                    parse_function_name,
                                    parse_seps,
                                    parse_functions_rule,
                                    pegtl::seq<parse_spaces, pegtl::one<')'>>,
                                    parse_seps> {};

struct grammar : pegtl::must<
                     parse_entry_point_rule> {
};


/*
 * templates
 */
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
  // pegtl::standard_trace<grammar>(fileInput); run to debug program... 
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