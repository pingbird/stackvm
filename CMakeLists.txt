cmake_minimum_required(VERSION 3.15)
project(stackvm)

set(CMAKE_CXX_STANDARD 20)

find_package(LLVM REQUIRED CONFIG)

include_directories(/usr/include/llvm-11/)
include_directories(/usr/include/llvm-c-11/)
include_directories(third_party/clipp/include)
link_directories(/usr/lib/llvm-11/lib)
add_executable(stackvm main.cc src/ir.cc src/ir.h src/bf.cc src/bf.h src/lowering.cc src/lowering.h src/ir_print.h src/ir_print.cc src/opt.h src/opt_resolve_regs.cc src/opt_validate.cc src/opt_fold.cc src/opt_loop.cc src/opt_resolve_type.cc src/backend_llvm.cc src/jit.cc src/backend_llvm.h src/jit.h src/diagnostics.h src/bfvm.cc src/bfvm.h src/diagnostics.cc src/tape_memory.cc src/tape_memory.h src/aot.h src/opt_cse.cc)

if (DEFINED BF_SANITIZE)
    target_link_libraries(stackvm asan)
    add_compile_options(-fsanitize=address)
endif()

if (DEFINED BF_NO_DIAG)
    ADD_DEFINITIONS(-DNDIAG)
endif()

target_link_libraries(stackvm LLVM-11)

add_library(stackvm-runtime STATIC runtime.cpp)