#pragma once

#include <L2.h>
#include <parser.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
namespace pegtl = TAO_PEGTL_NAMESPACE;
using namespace pegtl;

namespace L2 {
f_spilled compute_coloring(f &function, interference &interference_result) {
  int num_args = 0;
  var spilled_arg_1;
  var spilled_arg_2;
  return f_spilled{
      function.name,
      num_args,
      function.instructions,
      spilled_arg_1,
      spilled_arg_2};
};

} // namespace L2