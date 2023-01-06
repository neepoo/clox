#include "common.h"
#include "chunk.h"
#include "vm.h"


int main(int argc, const char *argv[]) {
    initVM();
    Chunk chunk;
    initChunk(&chunk);
    int constants = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constants, 123);
    constants = addConstant(&chunk, 3.4);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constants, 123);
    writeChunk(&chunk, OP_ADD, 123);
    constants = addConstant(&chunk, 5.6);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constants, 123);
    writeChunk(&chunk, OP_DIVIDE, 123);
    writeChunk(&chunk, OP_NEGATE, 123);
    writeChunk(&chunk, OP_RETURN, 123);
    interpret(&chunk);
    freeVM();
    freeChuck(&chunk);

    return 0;
}
