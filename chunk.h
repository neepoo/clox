// a module to define out code representation
// using "chunk" to refer to a sequence of bytecode
#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

// Each opcode determines how many operand bytes it has and what they mean.
// For example, a simple operation like “return” may have no operands,
// where an instruction for “load local variable” needs an operand to identify which variable to load.
// Each time we add a new opcode to clox, we specify what its operands look like—its instruction format.

typedef enum {
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN, // "return from the current function"
} OpCode;

// bytecode is a series of instruction, we'll store some other data along with
// the instruction, create a struct to hold it

typedef struct {
    int count;
    int capacity;
    uint8_t *code;  // a simple wrapper of dynamic array
    int* lines;  // Every time we touch the code array, we make a corresponding change to the line number array,
    ValueArray constants;  // store the chuck's constants
} Chunk;


// declare a function to initialize a new chunk
void initChunk(Chunk *chunk);

void freeChuck(Chunk *chunk);

// append a byte to the end of chuck
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
#endif

