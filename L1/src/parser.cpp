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
  struct name : pegtl::seq<
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

  struct label : pegtl::seq<
                     pegtl::one<':'>,
                     name>
  {
  };

  struct function_name_rule : pegtl::seq<
                                  pegtl::one<'@'>,
                                  name>
  {
  };

  /*
   * Separators.
   */
  struct spaces : pegtl::star<
                      pegtl::sor<
                          pegtl::one<' '>,
                          pegtl::one<'\t'>>>
  {
  };

  struct seps : pegtl::star<
                    pegtl::seq<
                        spaces,
                        pegtl::eol>>
  {
  };

  struct entry_point_rule : pegtl::seq<
                                seps,
                                pegtl::seq<spaces, pegtl::one<'('>>,
                                seps,
                                function_name_rule,
                                seps,
                                pegtl::seq<spaces, pegtl::one<')'>>,
                                seps>
  {
  };

  struct grammar : pegtl::must<
                       entry_point_rule>
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
  struct action<function_name_rule>
  {
    template <typename Input>
    static void apply(const Input &in, p &p)
    {
      p.entryPointLabel = in.string();
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
    p program;
    parse<grammar, action>(fileInput, program);
  }
}