#include "algorithms/graphcoloring.h"
#include "algorithms/interference.h"
#include "algorithms/liveness.h"
#include "algorithms/spilling.h"
#include "codegen.h"
#include "parser.h"
#include <algorithm>
#include <assert.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include <utils.h>

void print_help(char *progName)
{
  std::cerr << "Usage: " << progName
            << " [-v] [-g 0|1] [-O 0|1|2] [-s] [-l] [-i] SOURCE" << std::endl;
  return;
}

int main(int argc, char **argv)
{
  auto enable_code_generator = true;
  auto spill_only = false;
  auto interference_only = false;
  auto liveness_only = false;
  int32_t optLevel = 3;

  /*
   * Check the compiler arguments.
   */
  Utils::verbose = false;
  if (argc < 2)
  {
    print_help(argv[0]);
    return 1;
  }
  int32_t opt;
  int64_t functionNumber = -1;
  while ((opt = getopt(argc, argv, "vg:O:slif:")) != -1)
  {
    switch (opt)
    {

    case 'l':
      liveness_only = true;
      break;

    case 'i':
      interference_only = true;
      break;

    case 's':
      spill_only = true;
      break;

    case 'O':
      optLevel = strtoul(optarg, NULL, 0);
      break;

    case 'f':
      functionNumber = strtoul(optarg, NULL, 0);
      break;

    case 'g':
      enable_code_generator = (strtoul(optarg, NULL, 0) == 0) ? false : true;
      break;

    case 'v':
      Utils::verbose = true;
      break;

    default:
      print_help(argv[0]);
      return 1;
    }
  }

  /*
   * Parse the input file.
   */
  L3::p p;
  L3::pl1 pl1;
  L3::f function;
  L3::f_spilled function_spilled;
  /*
   * Print the source program.
   */
  if (spill_only)
  {

    /*
     * Parse an L2 function and the spill arguments.
     */
    function_spilled = L3::parse_function_spill(argv[optind]);
    functionNumber = 1;
  }
  else if (liveness_only)
  {

    /*
     * Parse an L2 function  */
    function = L3::parse_function(argv[optind]);
    functionNumber = 1;
  }
  else if (interference_only)
  {

    /*
     * Parse an L2 function.
     */
    function = L3::parse_function(argv[optind]);
    functionNumber = 1;
  }
  else
  {
    p = L3::parse_file(argv[optind]);
    L3::print_source_program(p);
    pl1.entry_point = p.entry_point;
    for (auto &fn : p.functions)
    {
      bool coloring_succeeds = true;
      int num_locals = 0;
      L3::fl1 result;
      L3::interference_colored interference_with_color_map;
      do
      { // run the loop until the graph coloring succeeds and we're done spilling variables
        L3::liveness liveness_result = L3::compute_liveness(fn);
        L3::interference interference_result = L3::compute_interference(fn, liveness_result);

        L3::f_spilled coloring_result = L3::compute_coloring(fn, interference_result, coloring_succeeds, num_locals, interference_with_color_map);

        if (!coloring_succeeds)
        {
          result = L3::compute_spilling(coloring_result, num_locals);
          fn = L3::f{result.name, result.num_args, result.instructions};
          num_locals++;
        }
        else
        {
          // if coloring succeeds, the first variable ready to color will not be spilled therefore I will use that for num_locals
          result = L3::fl1{
              coloring_result.name,
              coloring_result.num_args, std::stoll(coloring_result.spilled_arg1.name),
              coloring_result.instructions};
        };
      } while (!coloring_succeeds);
      // then collect the mapping between variables to registers and substitute
      remove_variables(result, interference_with_color_map);
      // replace any stack arg instruction with the transformation given by
      // t is equivalent to w <- mem rsp ? where ? is M plus the number of bytes of the stack space used for local variables
      for (auto &instr : result.instructions)
      {
        std::visit([&](auto &inst)
                   {
          using T = std::decay_t<decltype(inst)>;
          if constexpr (std::is_same_v<T, L3::StackArg>) {
            instr = L3::MemRead{
                inst.dest,
                L3::r::rsp,
                (inst.num.value + (8 * num_locals))};
          } else {
            instr = inst;
          }; },
                   instr);
      };
      pl1.functions.push_back(result);
    };
  }

  /*
   * Special cases.
   */
  if (spill_only)
  {

    L3::fl1 result = L3::compute_spilling(function_spilled, 0);
    L3::print_spilling(result);
    return 0;
  }

  /*
   * Liveness test.
   */
  if (liveness_only)
  {
    L3::liveness result = L3::compute_liveness(function);
    L3::print_liveness(result);
    return 0;
  }

  /*
   * Interference graph test.
   */
  if (interference_only)
  {
    L3::liveness liveness_result = L3::compute_liveness(function);
    L3::interference result = L3::compute_interference(function, liveness_result);
    L3::print_interference(result);
    return 0;
  }

  /*
   * Print a single L2 function case.
   */
  if (functionNumber != -1)
  {
    L3::print_function(function);
    return 0;
  }

  /*
   * Generate L1 code
   */
  if (enable_code_generator)
  {
    L3::generate_code(pl1);
  }

  return 0;
}
