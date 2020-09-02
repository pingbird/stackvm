# stackvm - A brainfuck VM

## Setup

Requirements:
* CMake
* LLVM
* A C++ compiler

Pull in submodules:
```
git submodule update --init
```

Build:

```
mkdir build
cd build
cmake ..
make -j12
```

## Usage

```
Usage:
    stackvm [-h] [-w <bits>] [-e <value>] [-l <size>] [-r <size>] [-p] [-d <dir>] <program>

Parameters:
    -h, --help               print this help message
    -w, --width <bits>       width of cells in bits, default = 8
    -e, --eof <value>        value of getchar when eof is reached, default = 0
    -l, --tape-left <size>   how much virtual memory to reserve to the left, default = 128MiB
    -r, --tape-right <size>  how much virtual memory to reserve to the right, default = 128MiB
    -p, --profile            enable profiling of build and execution
    -d, --dump <dir>         dumps intermediates into the specified folder
```

## Architecture

```
src/bfvm         - Public facing API
src/bf           - High level brainfuck (HBF) manipulation
src/ir           - SSA IR graph implementation and builder
src/ir_print     - Pretty printer for IR
src/lowering     - Lowers HBF into IR
src/opt          - Optimizations and visitors for IR
src/opt/fold         - Expression fold engine
src/opt/resolve_regs - Simple SSA register pruning 
src/opt/resolve_type - Lazy type resolution 
src/opt/validate     - Graph validator
src/backend_llvm - Translates StackVM IR to LLVM IR
src/jit          - Host JIT pipeline with LLVM
src/diagnostics  - DI for logging and artifact dumps
src/memory       - Lazy tape memory allocator
```