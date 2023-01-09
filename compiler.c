//
// Created by neepoo on 23-1-6.
//
#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

static void expression();


typedef struct {
    Token previous;
    Token current;
    bool hadError;
    bool panicMode;
} Parser;

// These are all of Lox’s precedence levels in order from lowest to highest.
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;
Parser parser;
Chunk *compilingChunk;

static Chunk *currentChunk() {
    return compilingChunk;
}


static void errorAt(Token *token, const char *msg) {
    if (parser.panicMode) return; //  The trick is that while the panic mode flag is set, we simply suppress any other errors that get detected.
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    // WHY？这里的msg不是\0结尾的str，这样打印不会有问题吗？
    fprintf(stderr, ": %s\n", msg);
    parser.hadError = true;
}

static void error(const char *msg) {
    errorAt(&parser.previous, msg);
}

static void errorAtCurrent(const char *msg) {
    errorAt(&parser.current, msg);
}

static void advance() {
    parser.previous = parser.current;
    // We keep looping, reading tokens and reporting the errors, until we hit a non-error one or reach the end.
    for (;;) {
        //  It asks the scanner for the next token and stores it for later use.
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        //  Remember, clox’s scanner doesn’t report lexical errors.
        //  Instead, it creates special error tokens and leaves it up to the parser to report them. We do that here.
        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char *msg) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(msg);
}

// It writes the given byte, which may be an opcode or an operand to an instruction.
static void emitByte(uint8_t byte) {
    // WHY?为什么这里是前一个token的line？而不是当前的token
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t) constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void expression();

static ParseRule *getRule(TokenType type);

static void parsePrecedence(Precedence precedence);


static void binary() {
    // When a prefix parser function is called, the leading token has already been consumed.
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return; // Unreachable.
    }
}

static void grouping() {
    /*
     * Again, we assume the initial
     * ( has already been consumed. We recursively call back into expression()
     * to compile the expression between the parentheses, then parse the closing ) at the end.
     */
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

/*
 * . We define a function for each expression that outputs the appropriate bytecode.
 * Then we build an array of function pointers.
 * The indexes in the array correspond to the TokenType enum values,
 * and the function at each index is the code to compile an expression of that token type.
 */

//To compile number literals, we store a pointer to the following function at the TOKEN_NUMBER index in the array.
static void number() {
    // We assume the token for the number literal has already been consumed and is stored in previous
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}


static void unary() {
    // The leading - token has been consumed and is sitting in parser.previous.
    TokenType operatorType = parser.previous.type;
    /*
     * It might seem a little weird to
     * write the negate instruction after its operand’s bytecode since the - appears on the left, but think about it in terms of order of execution:
     * 1. We evaluate the operand first which leaves its value on the stack.
     * 2. Then we pop that value, negate it, and push the result.
     *
     * So the OP_NEGATE instruction should be emitted last.
     * This is part of the compiler’s job—parsing the program in the order it appears in the source code
     * and rearranging it into the order that execution happens.
     */
    // Compile the operand
    parsePrecedence(PREC_UNARY);
    // emit the operator instruction
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return; // Unreachable.
    }
}

static void parsePrecedence(Precedence precedence) {
    // Let’s start with parsing prefix expressions.
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT]           = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
        [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH]         = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR]          = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG]          = {NULL, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, NULL, PREC_NONE},
        [TOKEN_GREATER]       = {NULL, NULL, PREC_NONE},
        [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
        [TOKEN_LESS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_LESS_EQUAL]    = {NULL, NULL, PREC_NONE},
        [TOKEN_IDENTIFIER]    = {NULL, NULL, PREC_NONE},
        [TOKEN_STRING]        = {NULL, NULL, PREC_NONE},
        [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL, NULL, PREC_NONE},
        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {NULL, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL, NULL, PREC_NONE},
        [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

static ParseRule *getRule(TokenType type) {
    return &rules[type];
}


static void expression() {
    // We simply parse the lowest precedence level, which subsumes all of the higher-precedence expressions too.
    parsePrecedence(PREC_ASSIGNMENT);
}
// A compiler has roughly two jobs. It parses the user’s source code to understand what it means.
// Then it takes that knowledge and outputs low-level instructions that produce the same semantics
bool compile(const char *source, Chunk *chunk) {
    // tine first phase of compilation is scanning
    initScanner(source);
    compilingChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    expression();
    consume(TOKEN_EOF, "Expected end of expression.");
    endCompiler();
    return !parser.hadError;
};