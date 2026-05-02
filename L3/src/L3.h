#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace L3
{
  // w, a, sx, x :: =
  enum class r
  {
    rax,
    rbx,
    rbp,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15, // w
    rdi,
    rsi,
    rdx,
    r8,
    r9,  // a
    rcx, // sx
    rsp  // x only
  };
  struct var
  {
    std::string name;
    bool operator<(const var &o) const { return name < o.name; }
    bool operator==(const var &o) const { return name == o.name; }
    bool operator!=(const var &o) const { return name != o.name; }
  }; // %name
  enum class aop
  {
    add,
    sub,
    mul,
    and_,
    lshift,
    rshift
  }; // += | -= | *= | &=
  enum class sop
  {
    lshift,
    rshift
  }; // <<= | >>=
  enum class cmp
  {
    lt,
    gt,
    le,
    eq
  }; // < | <= | =

  struct N
  {
    int64_t value;
  };
  struct l
  {
    std::string name;
  }; // l = @name
  struct label
  {
    std::string name;
  }; // label = :name

  // since t, s, and u are all supersets of w, and sx by transitivity and these are the operators actually used in the instructions
  // I will deviate from the math and add a var to these types.
  // w
  using w = std::variant<r, var>;
  // t ::= x | N
  using t = std::variant<r, N, var>;
  // s ::= t | label | l
  using s = std::variant<r, N, label, l, var>;
  // u ::= w | l
  using u = std::variant<r, l, var>;
  // callee, caller and arg register sets
  inline std::vector<w> callee_saved_registers = {
      r::r12,
      r::r13,
      r::r14,
      r::r15,
      r::rbp,
      r::rbx};
  inline std::vector<w> caller_saved_registers = {
      r::r10,
      r::r11,
      r::r8,
      r::r9,
      r::rax,
      r::rcx,
      r::rdi,
      r::rdx,
      r::rsi};
  inline std::vector<w> gp_registers =
      {r::r10,

       r::r11, r::r12, r::r13, r::r14, r::r15, r::r8, r::r9,
       r::rax, r::rbp, r::rbx, r::rcx, r::rdi, r::rdx, r::rsi};
  inline std::vector<r> colors =
      {r::r10,

       r::r11, r::r12, r::r13, r::r14, r::r15, r::r8, r::r9,
       r::rax, r::rbp, r::rbx, r::rcx, r::rdi, r::rdx, r::rsi};
  inline std::vector<w> arg_registers = {r::rdi, r::rsi, r::rdx, r::rcx, r::r8, r::r9};
  //  i ::=
  struct Assign
  {
    w dest;
    s src;
  }; // w <- s
  struct MemRead
  {
    w dest;
    w base;
    int64_t offset;
  }; // w <- mem x M
  struct MemWrite
  {
    w base;
    int64_t offset;
    s src;
  }; // mem x M <- s
  struct AopInst
  {
    w dest;
    aop op;
    t src;
  }; // w aop t
  struct SopReg
  {
    w dest;
    sop op;
    t shift;
  }; // w sop sx
  struct SopNum
  {
    w dest;
    sop op;
    N shift;
  }; // w sop N
  struct MemAopWrite
  {
    w base;
    int64_t offset;
    aop operation;
    t src;
  }; // mem x M += t
  struct MemAopRead
  {
    w dest;
    aop operation;
    w base;
    int64_t offset;
  }; // w += mem x M
  struct Compare
  {
    w dest;
    t left;
    cmp op;
    t right;
  }; // w <- t cmp t
  struct Cjump
  {
    t left;
    cmp op;
    t right;
    label target;
  }; // cjump t cmp t label
  struct Goto
  {
    label target;
  };
  struct LabelInst
  {
    label lbl;
  };
  struct Return
  {
  };
  struct ReturnT
  {
    t return_thing;
  };
  struct Call
  {
    u callee;
    N num_args;
  };

  struct CallPrint
  {
  };
  struct CallInput
  {
  };
  struct CallAllocate
  {
  };
  struct CallTupleError
  {
  };
  struct CallTensorError
  {
    int64_t F;
  }; // F ::= 1 | 3 | 4
  struct Increment
  {
    w reg;
  }; // w++
  struct Decrement
  {
    w reg;
  }; // w--
  struct LoadAddr
  {
    w dest;
    w base;
    w index;
    int64_t scale;
  }; // w @ w w E
  struct StackArg
  {
    w dest;
    N num;
  }; // w <- stack-arg M
  struct AopL3
  {
    w dest;
    t left;
    aop op;
    t right;
  }; // w <- t + t
  using callee_type = std::variant<u, CallPrint, CallInput, CallAllocate,
                                   CallTupleError, CallTensorError>;
  struct CallL3
  {
    callee_type callee;
    std::vector<t> args;
  };
  struct CallAssignL3
  {
    var dest;
    CallL3 callexpr;
  };
  struct brlabel
  {
    l label;
  };
  struct brtlabel
  {
    t cond;
    l label;
  };
  struct StoreL3
  {
    var dest;
    t src;
  }; // store %var <- t
  struct LoadL3
  {
    var dest;
    var src;
  }; // %dest <- load %src
  using i = std::variant<
      Assign, MemRead, MemWrite,
      AopInst, SopReg, SopNum,
      MemAopWrite, MemAopRead,
      Compare, Cjump, Goto, LabelInst,
      Return, ReturnT, Call, CallPrint, CallInput,
      CallAllocate, CallTupleError, CallTensorError,
      Increment, Decrement, LoadAddr, StackArg,
      AopL3, CallL3, CallAssignL3,
      StoreL3, LoadL3, brlabel, brtlabel>;

  // fl2 ::= (l N N i+)
  struct f
  {
    l name; // @name
    int64_t num_args;
    std::vector<i> instructions;
  };
  // fl2 ::= (l N N i+)
  struct fl2
  {
    l name; // @name
    int64_t num_args;
    std::vector<i> instructions;
  };
  // fl3::= define l ( vars ) { i+ }
  struct fl3
  {
    l name;
    std::vector<var> vars;
    std::vector<i> instructions;
  };
  // fl1 ::= (l N N i+)
  struct fl1
  {
    l name; // @name
    int64_t num_args;
    int64_t num_locals;
    std::vector<i> instructions;
  };
  struct f_spilled
  {
    l name; // @name
    int64_t num_args;
    std::vector<i> instructions;
    var spilled_arg1;
    var spilled_arg2;
  };
  // pl2 ::= (l f+)
  struct p
  {
    l entry_point; // @name
    std::vector<f> functions;
  };
  // pl2 ::= (l f+)
  struct pl2
  {
    l entry_point; // @name
    std::vector<fl2> functions;
  };
  struct pl3
  {
    std::vector<fl3> functions;
  };

  struct liveness
  {
    std::vector<std::vector<w>> in;
    std::vector<std::vector<w>> out;
    std::vector<std::vector<w>> kill;
  };
  struct interference
  {
    std::vector<std::vector<w>> array;
  };

  struct interference_colored
  {
    // each node in the interference graph will be paired with a register. color
    std::vector<std::vector<std::pair<w, r>>> array;
  };

  struct pl1
  {
    l entry_point; // @name
    std::vector<fl1> functions;
  };
  template <typename V>
  bool is_real_var(const V &val)
  {
    return std::visit([&](const auto &x) -> bool
                      {
    // if the value in the instruction is a variable, compare against variable of interest
    using X = std::decay_t<decltype(x)>;
    if constexpr (std::is_same_v<X, var>)
      return true;
    else
      return false; },
                      val);
  };
} // namespace L3