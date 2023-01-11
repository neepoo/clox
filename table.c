//
// Created by neepoo on 23-1-11.
//

#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

// This is how we manage the table’s load factor.
// We don’t grow when the capacity is completely full. Instead,
// we grow the array before then, when the array becomes at least 75% full.
#define TABLE_MAX_LOAD 0.75

Entry *findEntry(Entry *entries, int capacity, ObjString *pString);

void initTable(Table *table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void freeTable(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    initTable(table);
}

/*
 * Most of the interesting logic is in findEntry() which we’ll get to soon.
 * That function’s job is to take a key and figure out which bucket in the array it should go in.
 * It returns a pointer to that bucket—the address of the Entry in the array.
 */
Entry *findEntry(Entry *entries, int capacity, ObjString *key) {
    uint32_t index = key->hash % capacity;
    // When we are following a probe sequence during a lookup, and we hit a tombstone, we note it and keep going.
    Entry *tombstone = NULL;
    for (;;) {
        // 这里不会和所有的bucket都冲突，
        // 因为我们的负载因子是0.75,确保了桶总是有空的地方的。
        Entry *entry = &entries[index];
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // Empty entry.
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            // We found the key.
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

//Before we can put entries in the hash table, we do need a place to actually store them. We need to allocate an array of buckets.
static void adjustCapacity(Table *table, int capacity) {
    // 如果满了，底层调用realloc（NULL）
    Entry *entries = ALLOCATE(Entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    //Those new buckets may have new collisions that we need to deal with.
    // So the simplest way to get every entry where it belongs is to rebuild the table from scratch
    // by re-inserting every entry into the new empty array.

    //  We walk through the old array front to back.
    //  Any time we find a non-empty bucket, we insert that entry into the new array
    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) continue;
        Entry *dest = findEntry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    // After that’s done, we can release the memory for the old array.
    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

bool tableSet(Table *table, ObjString *key, Value value) {
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table, capacity);
    }
    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    // taking care to not increase the count if we overwrote the value for an already-present key.
    if (isNewKey && IS_NIL(entry->value)) table->count++;
    entry->key = key;
    entry->value = value;
    return isNewKey;
}

void tableAddAll(Table *from, Table *to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry *entry = &from->entries[i];
        //  Whenever it finds a non-empty bucket,
        //  it adds the entry to the destination hash table using the tableSet() function we recently defined.
        if (entry->key != NULL) {
            tableSet(to, entry->key, entry->value);
        }
    }
}

ObjString *tableFindString(Table *table, const char *chars,
                           int length, uint32_t hash) {
    if (table->count == 0) return NULL;
    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry *entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length &&
                   entry->key->hash == hash &&
                   memcmp(entry->key->chars, chars, length) == 0) {
            // We found it.
            return entry->key;
        }
        index = (index + 1) % table->capacity;
    }
}

bool tableGet(Table *table, ObjString *key, Value *value) {
    if (table->capacity == 0) return false;
    Entry const *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;
    *value = entry->value;
    return true;
}

bool tableDelete(Table *table, ObjString *key) {
    if (table->count == 0) return false;
    // Find the entry
    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;
    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);
    return true;
};