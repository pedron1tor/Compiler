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

void print_help(char *progName) {
  std::cerr << "Usage: " << progName
            << " [-v] [-g 0|1] [-O 0|1|2] [-s] [-l] [-i] SOURCE" << std::endl;
  return;
}

int main(int argc, char **argv) {
  auto enable_code_generator = true;
  auto spill_only = false;
  auto interference_only = false;
  auto liveness_only = false;
  int32_t optLevel = 3;

  /*
   * Check the compiler arguments.
   */
  Utils::verbose = false;
  if (argc < 2) {
    print_help(argv[0]);
    return 1;
  }
  int32_t opt;
  int64_t functionNumber = -1;
  while ((opt = getopt(argc, argv, "vg:O:slif:")) != -1) {
    switch (opt) {

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
  L2::p p;
  L2::pfl1 pfl1;
  L2::f function;
  L2::f_spilled function_spilled;
  /*
   * Print the source program.
   */
  if (spill_only) {

    /*
     * Parse an L2 function and the spill arguments.
     */
    function_spilled = L2::parse_function_spill(argv[optind]);
    functionNumber = 1;

  } else if (liveness_only) {

    /*
     * Parse an L2 function  */
    function = L2::parse_function(argv[optind]);
    functionNumber = 1;
  } else if (interference_only) {

    /*
     * Parse an L2 function.
     */
    function = L2::parse_function(argv[optind]);
    functionNumber = 1;

  } else {
    p = L2::parse_file(argv[optind]);
    L2::print_source_program(p);
    }

  /*
   * Special cases.
   */
  if (spill_only) {

    L2::fl1 result = L2::compute_spilling(function_spilled);
    L2::print_spilling(result);
    return 0;
  }

  /*
   * Liveness test.
   */
  if (liveness_only) {
    L2::liveness result = L2::compute_liveness(function);
    L2::print_liveness(result);
    return 0;
  }

  /*
   * Interference graph test.
   */
  if (interference_only) {
    L2::liveness liveness_result = L2::compute_liveness(function);
    L2::interference result = L2::compute_interference(function, liveness_result);
    L2::print_interference(result);
    return 0;
  }

  /*
   * Print a single L2 function case.
   */
  if (functionNumber != -1) {
    L2::print_function(function);
    return 0;
  }

  /*
   * Generate L1 code
   */
  if (enable_code_generator) {
    L2::generate_code(pfl1);
  }

  return 0;
}
