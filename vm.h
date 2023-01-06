//
// Created by neepoo on 23-1-6.
//

#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256
typedef struct {
    Chunk *chunk;
    uint8_t *ip;  // it keeps tracks of where it is the location of the instruction currently being executed
    Value stack[STACK_MAX];  // index 0 refer stack bottom
    Value *stackTop;  // 后续的操作都是对stackTop指针进行的，而不是进行数组索引
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

void initVM();

void freeVM();

InterpretResult interpret(Chunk *chunk);

void push(Value value);

Value pop();

#endif
