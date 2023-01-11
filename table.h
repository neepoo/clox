//
// Created by neepoo on 23-1-11.
//

#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

// It’s a simple key/value pair. Since the key is always a string,
// we store the ObjString pointer directly instead of wrapping it in a Value. It’s a little faster and smaller this way.
typedef struct {
    ObjString *key;
    Value value;
} Entry;

/*
 * A hash table is an array of entries. As in our dynamic array earlier,
 * we keep track of both the allocated size of the array (capacity) and
 * the number of key/value pairs currently stored in it (count).
 * The ratio of count to capacity is exactly the load factor of the hash table.
 */
typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Table;

void initTable(Table *table);

void freeTable(Table *table);

bool tableGet(Table *table, ObjString *key, Value *value);

bool tableSet(Table *table, ObjString *key, Value value);

bool tableDelete(Table *table, ObjString *key);

void tableAddAll(Table *from, Table *to);

ObjString *tableFindString(Table *table, const char *chars,
                           int length, uint32_t hash);

#endif