//
// Created by neepoo on 23-1-11.
//
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *) reallocate(NULL, 0, size);
    object->type = type;
    /*
     * Since this is a singly linked list, the easiest place to insert it is as the head.
     * That way, we don’t need to also store a pointer to the tail and keep it updated.
     * 先分配的对象在连表的尾部，后分配的对象在连表的头部
     */
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

// allocating the string and copying its characters over is already an O(n) operation,
// so it’s a good time to also do the O(n) calculation of the string’s hash.
// The fun happens over at the callers. allocateString() is called from two places:
// the function that copies a string and
// the one that takes ownership of an existing dynamically allocated string. We’ll start with the first.
static ObjString *allocateString(char *chars, int length, uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    tableSet(&vm.strings, string, NIL_VAL);
    return string;
}

// 字符串hash
//This is the actual bona fide “hash function” in clox.
// The algorithm is called “FNV-1a”, and is the shortest decent hash function I know.
// Brevity is certainly a virtue in a book that aims to show you every line of code.
static uint32_t hashString(const char *key, int length) {
    // ou start with some initial hash value, usually a constant with certain carefully chosen mathematical properties.
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);
};

ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
};

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
};