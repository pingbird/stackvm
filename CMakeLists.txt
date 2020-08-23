cmake_minimum_required(VERSION 3.15)
project(bfvm)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")

add_executable(bfvm main.cpp src/ir.cpp src/ir.h src/bf.cpp src/bf.h src/lowering.cpp src/bf2ir.cpp src/bf2ir.h src/lowering.h src/constants.h src/ir_print.h src/ir_print.cpp src/opt.h src/opt/resolve_regs.cpp src/opt/validate.cpp src/opt/fold.cpp)