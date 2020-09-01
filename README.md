# stackvm - A brainfuck VM

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
src/math_util    - Misc. math utilities
```