#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace L2 {
// w, a, sx, x :: =
enum class r {
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
struct var {
  std::string name;
}; // %name
enum class aop { add,
                 sub,
                 mul,
                 and_ }; // += | -= | *= | &=
enum class sop { lshift,
                 rshift }; // <<= | >>=
enum class cmp { lt,
                 le,
                 eq }; // < | <= | =

struct N {
  int64_t value;
};
struct l {
  std::string name;
}; // l = @name
struct label {
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

//  i ::=
struct Assign {
  w dest;
  s src;
}; // w <- s
struct MemRead {
  r dest;
  r base;
  int64_t offset;
}; // w <- mem x M
struct MemWrite {
  r base;
  int64_t offset;
  s src;
}; // mem x M <- s
struct AopInst {
  w dest;
  aop op;
  t src;
}; // w aop t
struct SopReg {
  r dest;
  sop op;
  t shift;
}; // w sop sx
struct SopNum {
  r dest;
  sop op;
  N shift;
}; // w sop N
struct MemAopWrite {
  r base;
  int64_t offset;
  aop operation;
  t src;
}; // mem x M += t
struct MemAopRead {
  r dest;
  aop operation;
  r base;
  int64_t offset;
}; // w += mem x M
struct Compare {
  r dest;
  t left;
  cmp op;
  t right;
}; // w <- t cmp t
struct Cjump {
  t left;
  cmp op;
  t right;
  label target;
}; // cjump t cmp t label
struct Goto {
  label target;
};
struct LabelInst {
  label lbl;
};
struct Return {};
struct Call {
  u callee;
  N num_args;
};
struct CallPrint {};
struct CallInput {};
struct CallAllocate {};
struct CallTupleError {};
struct CallTensorError {
  int64_t F;
}; // F ::= 1 | 3 | 4
struct Increment {
  r reg;
}; // w++
struct Decrement {
  r reg;
}; // w--
struct LoadAddr {
  r dest;
  r base;
  r index;
  int64_t scale;
}; // w @ w w E

using i = std::variant<
    Assign, MemRead, MemWrite,
    AopInst, SopReg, SopNum,
    MemAopWrite, MemAopRead,
    Compare, Cjump, Goto, LabelInst,
    Return, Call, CallPrint, CallInput,
    CallAllocate, CallTupleError, CallTensorError,
    Increment, Decrement, LoadAddr>;

// f ::= (l N N i+)
struct f {
  l name; // @name
  int64_t num_args;
  std::vector<i> instructions;
};

// p ::= (l f+)
struct p {
  l entry_point; // @name
  std::vector<f> functions;
};

p parse_file(char *filename);

} // namespace L2