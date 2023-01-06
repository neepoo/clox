cmake_minimum_required(VERSION 3.24)
project(lox_virtual_machine C)

set(CMAKE_C_STANDARD 99)

add_executable(lox_virtual_machine main.c common.h chunk.h chunk.c memory.c memory.h debug.c debug.h value.c value.h vm.c vm.h)
