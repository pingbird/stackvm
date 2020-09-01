cmake_minimum_required(VERSION 3.15)
project(bfvm)

set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED CONFIG)

add_compile_options(-g -fno-exceptions)
include_directories(/usr/include/llvm-10/)
include_directories(/usr/include/llvm-c-10/)
link_directories(/usr/lib/llvm-10/lib)
add_executable(bfvm main.cc src/ir.cc src/ir.h src/bf.cc src/bf.h src/lowering.cc src/lowering.h src/common.h src/ir_print.h src/ir_print.cc src/opt.h src/opt/resolve_regs.cc src/opt/validate.cc src/opt/fold.cc src/opt/fold.h src/opt/resolve_type.cc src/backend_llvm.cc src/jit.cc src/backend_llvm.h src/jit.h src/diagnostics.h src/bfvm.cpp src/bfvm.h)

if (DEFINED BF_SANITIZE)
    target_link_libraries(bfvm asan)
    add_compile_options(-fsanitize=address)
endif()

target_link_libraries(bfvm LLVM-10)