cmake_minimum_required(VERSION 3.15)
project(bfvm)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

add_executable(bfvm main.cc src/ir.cc src/ir.h src/bf.cc src/bf.h src/lowering.cc src/lowering.h src/common.h src/ir_print.h src/ir_print.cc src/opt.h src/opt/resolve_regs.cc src/opt/validate.cc src/opt/fold.cc src/opt/fold.h src/opt/type_resolution.cc)