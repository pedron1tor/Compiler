# Compiler

A multi-stage compiler, written in C++17, that translates a small C-like language down to x86-64 assembly. Instead of one big pass, the compiler is a chain of intermediate languages. Each stage lowers one language into the next, simpler one:

```
LB ──► LA ──► IR ──► L3 ──► L2 ──► L1 ──► x86-64
```

Each stage is a standalone compiler in its own directory. It has a PEG grammar (built with [PEGTL](https://github.com/taocpp/PEGTL)), an AST, and a code generator that emits the next language down.

## Stages

| Stage | Input language | Main work |
|-------|----------------|-----------|
| **LB → LA** | Structured language with scopes, `while`, `if`/`else`, `break`/`continue` | Scope resolution and renaming, lowering structured control flow to labels and branches |
| **LA → IR** | Typed language with arrays, tuples and functions | Value encoding for integers and pointers, array bounds checks, explicit basic blocks |
| **IR → L3** | Basic-block IR with typed variables and multi-dimensional arrays | Linearizing blocks and computing array address arithmetic |
| **L3 → L2** | Three-address code with variables | Instruction selection by tree tiling, calling-convention lowering |
| **L2 → L1** | L1 extended with unlimited variables | Liveness analysis, interference graph, graph-coloring register allocation, spilling |
| **L1 → x86-64** | Register-level language that closely mirrors x86-64 | Assembly emission (AT&T syntax) |

### Register allocation (L2)

The L2 stage is the most algorithm-heavy part of the project (`L2/src/algorithms/`):

- **Liveness**: iterative dataflow analysis that computes GEN/KILL and IN/OUT sets for each instruction
- **Interference**: builds the interference graph from liveness results and adds constraints for fixed registers
- **Graph coloring**: assigns x86-64 registers so that no two interfering variables share one
- **Spilling**: when coloring fails, picks the variable with the highest degree, rewrites it to go through stack memory, and tries again

The L2 compiler can print each analysis on its own:

```sh
L2/bin/L2 -l prog.L2   # liveness
L2/bin/L2 -i prog.L2   # interference graph
L2/bin/L2 -s prog.L2   # spill
```

## Building

The only dependency is PEGTL, which is included under `lib/`.

```sh
make            # build every stage
cd L2 && make   # build a single stage
```

Each stage's compiler ends up in `<stage>/bin/<stage>`. Running it on a source file writes the next language down to `prog.<next>`:

```sh
LB/bin/LB  program.LB   # -> prog.LA
LA/bin/LA  prog.LA      # -> prog.IR
...
L1/bin/L1  prog.L1      # -> prog.S
```

The generated assembly calls a small C runtime that provides `print`, `input` and `allocate`. That runtime isn't included in this repository, so you have to supply one to link and run the output.

## Layout

```
L1/ L2/ L3/ IR/ LA/ LB/   one compiler per stage
  src/parser.cpp          PEGTL grammar and AST construction
  src/parsing/            grammar rules and semantic actions
  src/codegen.cpp         lowering to the next stage
  mytests/                small hand-written test programs
lib/PEGTL/                parsing library (Boost Software License)
util/                     shared helpers
```
