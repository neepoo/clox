//
// Created by neepoo on 23-1-6.
//
#include <stdio.h>

#include "compiler.h"
#include "scanner.h"

void compile(const char* source){
    // tine first phase of compilation is scanning
    initScanner(source);
    int line = 1;
    for (;;) {
        Token token =scanToken();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", token.type, token.length, token.start);
        if (token.type == TOKEN_EOF) break;
    }
};