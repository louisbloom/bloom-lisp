#include "../include/lisp.h"
#include <stdint.h>
#include <string.h>

/* FNV-1a hash helpers */
static unsigned long fnv_init(void)
{
    return 2166136261UL;
}

static unsigned long fnv_byte(unsigned long hash, unsigned char byte)
{
    hash ^= byte;
    hash *= 16777619UL;
    return hash;
}

static unsigned long fnv_bytes(unsigned long hash, const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++) {
        hash = fnv_byte(hash, p[i]);
    }
    return hash;
}

static unsigned long fnv_string(unsigned long hash, const char *str)
{
    const unsigned char *p = (const unsigned char *)str;
    while (*p) {
        hash = fnv_byte(hash, *p++);
    }
    return hash;
}

/* Hash any LispObject key. Strings and symbols hash identically (interchangeable keys). */
static size_t hash_lisp_key(LispObject *key, size_t bucket_count)
{
    unsigned long hash = fnv_init();

    if (key == NIL) {
        hash = fnv_byte(hash, 0);
        return hash % bucket_count;
    }

    switch (key->type) {
    case LISP_STRING:
        hash = fnv_string(hash, key->value.string);
        break;
    case LISP_SYMBOL:
        /* Same hash as string for backward compatibility */
        hash = fnv_string(hash, key->value.symbol->name);
        break;
    case LISP_KEYWORD:
        hash = fnv_string(hash, key->value.symbol->name);
        break;
    case LISP_INTEGER:
        hash = fnv_byte(hash, 'I');
        hash = fnv_bytes(hash, &key->value.integer, sizeof(int64_t));
        break;
    case LISP_NUMBER:
        hash = fnv_byte(hash, 'N');
        hash = fnv_bytes(hash, &key->value.number, sizeof(double));
        break;
    case LISP_CHAR:
        hash = fnv_byte(hash, 'C');
        hash = fnv_bytes(hash, &key->value.character, sizeof(unsigned int));
        break;
    case LISP_BOOLEAN:
        hash = fnv_byte(hash, 'B');
        hash = fnv_byte(hash, key->value.boolean ? 1 : 0);
        break;
    default:
    {
        /* Compound types: hash by pointer identity */
        hash = fnv_byte(hash, 'P');
        uintptr_t ptr = (uintptr_t)key;
        hash = fnv_bytes(hash, &ptr, sizeof(uintptr_t));
        break;
    }
    }

    return hash % bucket_count;
}

/*
 * Compare two LispObject keys for hash table equality.
 * Strings and symbols are interchangeable (same namespace).
 * Primitive types use value equality. Compound types use pointer identity.
 */
int hash_keys_equal(LispObject *a, LispObject *b)
{
    if (a == b)
        return 1;
    if (a == NIL || b == NIL)
        return 0;

    /* String/symbol/keyword interop: extract text for comparison */
    const char *a_str = NULL, *b_str = NULL;
    if (a->type == LISP_STRING)
        a_str = a->value.string;
    else if (a->type == LISP_SYMBOL)
        a_str = a->value.symbol->name;
    if (b->type == LISP_STRING)
        b_str = b->value.string;
    else if (b->type == LISP_SYMBOL)
        b_str = b->value.symbol->name;

    if (a_str && b_str)
        return strcmp(a_str, b_str) == 0;
    if (a_str || b_str)
        return 0; /* One is string/symbol, other is not */

    /* Different types can't be equal (string/symbol already handled above) */
    if (a->type != b->type)
        return 0;

    switch (a->type) {
    case LISP_KEYWORD:
        return strcmp(a->value.symbol->name, b->value.symbol->name) == 0;
    case LISP_INTEGER:
        return a->value.integer == b->value.integer;
    case LISP_NUMBER:
        return a->value.number == b->value.number;
    case LISP_CHAR:
        return a->value.character == b->value.character;
    case LISP_BOOLEAN:
        return a->value.boolean == b->value.boolean;
    default:
        return 0; /* Compound types: pointer equality already checked above */
    }
}

/* Get entry from hash table */
struct HashEntry *hash_table_get_entry(LispObject *table, LispObject *key)
{
    if (table->type != LISP_HASH_TABLE) {
        return NULL;
    }

    size_t hash = hash_lisp_key(key, table->value.hash_table.bucket_count);
    struct HashEntry **buckets = (struct HashEntry **)table->value.hash_table.buckets;

    struct HashEntry *entry = buckets[hash];
    while (entry) {
        if (hash_keys_equal(entry->key, key)) {
            return entry;
        }
        entry = entry->next;
    }

    return NULL;
}

/* Set value in hash table */
struct HashEntry *hash_table_set_entry(LispObject *table, LispObject *key, LispObject *value)
{
    if (table->type != LISP_HASH_TABLE) {
        return NULL;
    }

    size_t hash = hash_lisp_key(key, table->value.hash_table.bucket_count);
    struct HashEntry **buckets = (struct HashEntry **)table->value.hash_table.buckets;

    /* Check if key exists */
    struct HashEntry *entry = buckets[hash];
    while (entry) {
        if (hash_keys_equal(entry->key, key)) {
            entry->value = value;
            return entry;
        }
        entry = entry->next;
    }

    /* Create new entry */
    entry = GC_malloc(sizeof(struct HashEntry));
    entry->key = key;
    entry->value = value;
    entry->next = buckets[hash];
    buckets[hash] = entry;

    table->value.hash_table.entry_count++;

    return entry;
}

/* Remove entry from hash table */
int hash_table_remove_entry(LispObject *table, LispObject *key)
{
    if (table->type != LISP_HASH_TABLE) {
        return 0;
    }

    size_t hash = hash_lisp_key(key, table->value.hash_table.bucket_count);
    struct HashEntry **buckets = (struct HashEntry **)table->value.hash_table.buckets;

    struct HashEntry *entry = buckets[hash];
    struct HashEntry *prev = NULL;

    while (entry) {
        if (hash_keys_equal(entry->key, key)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                buckets[hash] = entry->next;
            }
            table->value.hash_table.entry_count--;
            return 1;
        }
        prev = entry;
        entry = entry->next;
    }

    return 0;
}

/* Clear all entries from hash table */
void hash_table_clear(LispObject *table)
{
    if (table->type != LISP_HASH_TABLE) {
        return;
    }

    struct HashEntry **buckets = (struct HashEntry **)table->value.hash_table.buckets;

    for (size_t i = 0; i < table->value.hash_table.bucket_count; i++) {
        struct HashEntry *entry = buckets[i];
        while (entry) {
            struct HashEntry *next = entry->next;
            /* GC will handle freeing */
            entry = next;
        }
        buckets[i] = NULL;
    }

    table->value.hash_table.entry_count = 0;
}
