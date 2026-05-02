#include "L3.h"
#include "parser.h"
#include <fstream>

namespace L3
{
  s t_to_s(t &t_item)
  {
    return std::visit([](auto &v) -> s
                      { return s{v}; }, t_item);
  }
  s w_to_s(w &w_item)
  {
    return std::visit([](auto &v) -> s
                      { return s{v}; }, w_item);
  }

  std::vector<i>
  args_to_regs(std::vector<var> &variables)
  {
    // allocate the variables to the registers first, then if still remaining allocate stack space.
    std::vector<i> allocating_instructions;
    for (int reg_idx = 0; reg_idx < (int)std::min(6UL, variables.size()); reg_idx++)
    {
      allocating_instructions.push_back(Assign{w{variables[reg_idx]}, w_to_s(arg_registers[reg_idx])});
    }
    // continue allocating on the stack if more than 6 args
    if (variables.size() > 6)
    {
      // TODO allocate stack space for the remaning variables
    }
    return allocating_instructions;
  };
  fl2 fl3_to_fl2(fl3 &f)
  {
    fl2 new_function;
    new_function.name = f.name;
    new_function.num_args = f.vars.size();
    // assign a register for each of the parameters
    std::vector<i> register_assigns = args_to_regs(f.vars);
    // append to the instructions of the function.
    new_function.instructions.insert(new_function.instructions.end(), register_assigns.begin(), register_assigns.end());

    for (auto &instr : f.instructions)
    {
      std::visit([&](auto &inst)
                 {
          using T = std::decay_t<decltype(inst)>;
          if constexpr (std::is_same_v<T, L3::ReturnT>) {
            new_function.instructions.push_back(Assign{r::rax, t_to_s(inst.return_thing)});
            new_function.instructions.push_back(Return{});
          } else if constexpr (std::is_same_v<T, L3::AopL3>) {
            new_function.instructions.push_back(Assign{inst.dest, t_to_s(inst.left)});
            new_function.instructions.push_back(AopInst{inst.dest, inst.op, inst.right});
          } else {
             new_function.instructions.push_back(inst);
          }; },
                 instr);
    };
    return new_function;
  };
  pl2 pl3_to_pl2(pl3 &p)
  {
    std::string entry_point = p.functions[0].name.name;
    std::vector<fl2> function_list;
    for (auto &fn : p.functions)
    {
      function_list.push_back(fl3_to_fl2(fn));
    }
    return pl2{
        entry_point, function_list};
  };
  void generate_code(pl3 &p)
  {
    pl2 transformed_p;
    transformed_p = pl3_to_pl2(p);
    std::ofstream outputFile;
    outputFile.open("prog.L2");

    outputFile << "(" << transformed_p.entry_point.name << "\n";
    // build the L2 code for each of the funtions inside of the ast/program
    for (auto &fn : transformed_p.functions)
    {
      print_functionl2(fn, true, &outputFile);
    };
    outputFile << ")\n";

    outputFile.close();
  }
} // namespace L3