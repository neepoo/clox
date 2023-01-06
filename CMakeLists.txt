cmake_minimum_required(VERSION 3.24)
project(clox C)

set(CMAKE_C_STANDARD 99)

add_executable(lox_virtual_machine main.c common.h chunk.h chunk.c memory.c memory.h debug.c debug.h value.c value.h vm.c vm.h compiler.c compiler.h scanner.c scanner.h)
