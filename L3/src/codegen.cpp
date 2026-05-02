#include "L3.h"
#include "parser.h"
#include <fstream>
#include <unordered_map>
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
    int stack_arg_counter = 0;
    if (variables.size() > 6)
    {
      for (int reg_idx = 6; reg_idx < variables.size(); reg_idx++)
      {
        // allocate stack space for the remaning variables
        allocating_instructions.push_back(StackArg{w{
                                                       variables[reg_idx]},
                                                   N{stack_arg_counter}});
        stack_arg_counter += 8;
      };
    };

    return allocating_instructions;
  };
  label check_map_and_rename_label(std::unordered_map<label, label> &map, label &lbl, int &global_counter)
  {
    auto it = map.find(lbl);
    label new_label = label{lbl.name + "_global_" + std::to_string(global_counter)};
    if (it != map.end())
    {
      std::string s = it->second.name;
      int number = std::stoi(s.substr(s.rfind('_') + 1));
      if (number == global_counter)
      {
        return label{s};
      }
      else
      {
        map[lbl] = new_label;
      };
    }
    else
    {
      map[new_label] = new_label;
      return new_label;
    };
  };

  fl2 fl3_to_fl2(fl3 &f, std::unordered_map<label, label> &label_map, int &lbl_counter)
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
          } else if constexpr (std::is_same_v<T, L3::LabelInst>)
          {
            new_function.instructions.push_back(LabelInst{check_map_and_rename_label(label_map, inst.lbl, lbl_counter)}); 
          } else if constexpr (std::is_same_v<T, L3::brlabel>){
            new_function.instructions.push_back(brlabel{check_map_and_rename_label(label_map, inst.lbl, lbl_counter)});
          }
          else if constexpr (std::is_same_v<T, brtlabel>) {
            new_function.instructions.push_back(brtlabel{inst.cond, check_map_and_rename_label(label_map, inst.lbl, lbl_counter)});
          } else if constexpr (std::is_same_v<T, StoreL3>) {                                                                                                                           
      if (std::holds_alternative<label>(inst.src)) {      
        // if store.src is a label, then check_map and rename label                                                                                                                   
          label renamed = check_map_and_rename_label(label_map, std::get<label>(inst.src), lbl_counter);                                                                     
          new_function.instructions.push_back(StoreL3{inst.dest, s{renamed}});                                                                                               
      } else {                                                                                                                                                             
          new_function.instructions.push_back(inst);                                                                                                                         
      }                                                                                                                                                                    
  }
  else if constexpr (std::is_same_v<T, Assign>) {        
    // if assign.src is a label, then check_map and rename label                                                                                                                    
      if (std::holds_alternative<label>(inst.src)) {
          label renamed = check_map_and_rename_label(label_map, std::get<label>(inst.src), lbl_counter);                                                                     
          new_function.instructions.push_back(Assign{inst.dest, s{renamed}});                                                                                                
      } else {
          new_function.instructions.push_back(inst);                                                                                                                         
      }                                                                                                                                                                      
  }
            
          else {
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
    int glob_lbl_counter = 0;
    // hash map, l3-labels -> l2-labels
    std::unordered_map<label, label> label_map;
    for (auto &fn : p.functions)
    {
      function_list.push_back(fl3_to_fl2(fn, label_map, glob_lbl_counter));
      glob_lbl_counter++;
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