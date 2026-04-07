#include <sched.h>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

#include <L1.h>
#include <parser.h>

namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L1
{
  /*
   * Grammar rules definitions from now on.
   */
  struct parse_name : pegtl::seq<
                          pegtl::plus<
                              pegtl::sor<
                                  pegtl::alpha,
                                  pegtl::one<'_'>>>,
                          pegtl::star<
                              pegtl::sor<
                                  pegtl::alpha,
                                  pegtl::one<'_'>,
                                  pegtl::digit>>>
  {
  };

  struct parse_label : pegtl::seq<
                           pegtl::one<':'>,
                           parse_name>
  {
  };

  struct parse_function_name_rule : pegtl::seq<
                                        pegtl::one<'@'>,
                                        parse_name>
  {
  };

  /*
   * Separators.
   */
  struct parse_spaces : pegtl::star<
                            pegtl::sor<
                                pegtl::one<' '>,
                                pegtl::one<'\t'>>>
  {
  };

  struct parse_seps : pegtl::star<
                          pegtl::seq<
                              parse_spaces,
                              pegtl::eol>>
  {
  };
  struct parse_instruction
  {
  };

  struct parse_instructions
  {
  };
  
  struct parse_number : pegtl::seq<pegtl::sor<
                  pegtl::seq<
                            pegtl::opt<pegtl::sor<pegtl::one<'+'>, pegtl::one<'-'>>>,
                            pegtl::range<'1', '9'>,
                            pegtl::star<pegtl::digit>
                            >, 
                            pegtl::one<'0'>
                            >>
  {
  };

  struct parse_function_rule : pegtl::seq<parse_seps,
                                          pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                          parse_seps,
                                          parse_function_name_rule,
                                          parse_seps,
                                          pegtl::seq<parse_spaces, parse_number>,
                                          parse_seps,
                                          pegtl::seq<parse_spaces, parse_number>,
                                          // parse_instructions,
                                          parse_seps,
                                          pegtl::seq<parse_spaces, pegtl::one<')'>>,
                                          parse_seps>
  {
  };
  struct parse_functions_rule : pegtl::seq<
   pegtl::plus<parse_function_rule>
   >
  {
   
  };
  struct parse_entry_point_rule : pegtl::seq<
                                      parse_seps,
                                      pegtl::seq<parse_spaces, pegtl::one<'('>>,
                                      parse_seps,
                                      parse_function_name_rule,
                                      parse_seps, 
                                      parse_functions_rule,
                                      pegtl::seq<parse_spaces, pegtl::one<')'>>,
                                      parse_seps>
  {
  };

  struct grammar : pegtl::must<
                       parse_entry_point_rule>
  {
  };

  /*
   * Actions attached to grammar rules.
   */
  template <typename Rule>
  struct action : pegtl::nothing<Rule>
  {
  };

  template <>
  struct action<parse_function_name_rule>
  {
    template <typename Input>
    static void apply(const Input &in, p &program)
    {
      if (program.entry_point.name.empty())
      {
        program.entry_point.name = in.string(); 
      }
      else {
          f newF;
          newF.name.name = in.string();
          program.functions.push_back(newF);
      }
    }
  };

  p parse_file(char *filename)
  {
    /*
     * Check the grammar for some possible issues.
     */
    if (pegtl::analyze<grammar>() != 0)
    {
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
}