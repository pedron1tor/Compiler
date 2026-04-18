#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
f compute_spilling(f_spilled &function) {

  return f{function.name, function.num_args, function.instructions};
};
void print_spilling(f result) {
  print_function(result);
};

} // namespace L2