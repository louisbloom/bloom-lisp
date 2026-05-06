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

    switch (LISP_TYPE(key)) {
    case LISP_STRING:
        hash = fnv_string(hash, LISP_STR_VAL(key));
        break;
    case LISP_SYMBOL:
        /* Same hash as string for backward compatibility */
        hash = fnv_string(hash, LISP_SYM_VAL(key)->name);
        break;
    case LISP_KEYWORD:
        hash = fnv_string(hash, LISP_SYM_VAL(key)->name);
        break;
    case LISP_INTEGER:
        hash = fnv_byte(hash, 'I');
        hash = fnv_bytes(hash, &LISP_INT_VAL(key), sizeof(int64_t));
        break;
    case LISP_NUMBER:
        hash = fnv_byte(hash, 'N');
        hash = fnv_bytes(hash, &LISP_NUM_VAL(key), sizeof(double));
        break;
    case LISP_CHAR:
        hash = fnv_byte(hash, 'C');
        hash = fnv_bytes(hash, &LISP_CHAR_VAL(key), sizeof(unsigned int));
        break;
    case LISP_BOOLEAN:
        hash = fnv_byte(hash, 'B');
        hash = fnv_byte(hash, LISP_BOOL_VAL(key) ? 1 : 0);
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
    if (LISP_TYPE(a) == LISP_STRING)
        a_str = LISP_STR_VAL(a);
    else if (LISP_TYPE(a) == LISP_SYMBOL)
        a_str = LISP_SYM_VAL(a)->name;
    if (LISP_TYPE(b) == LISP_STRING)
        b_str = LISP_STR_VAL(b);
    else if (LISP_TYPE(b) == LISP_SYMBOL)
        b_str = LISP_SYM_VAL(b)->name;

    if (a_str && b_str)
        return strcmp(a_str, b_str) == 0;
    if (a_str || b_str)
        return 0; /* One is string/symbol, other is not */

    /* Different types can't be equal (string/symbol already handled above) */
    if (LISP_TYPE(a) != LISP_TYPE(b))
        return 0;

    switch (LISP_TYPE(a)) {
    case LISP_KEYWORD:
        return strcmp(LISP_SYM_VAL(a)->name, LISP_SYM_VAL(b)->name) == 0;
    case LISP_INTEGER:
        return LISP_INT_VAL(a) == LISP_INT_VAL(b);
    case LISP_NUMBER:
        return LISP_NUM_VAL(a) == LISP_NUM_VAL(b);
    case LISP_CHAR:
        return LISP_CHAR_VAL(a) == LISP_CHAR_VAL(b);
    case LISP_BOOLEAN:
        return LISP_BOOL_VAL(a) == LISP_BOOL_VAL(b);
    default:
        return 0; /* Compound types: pointer equality already checked above */
    }
}

/* Get entry from hash table */
struct HashEntry *hash_table_get_entry(LispObject *table, LispObject *key)
{
    if (LISP_TYPE(table) != LISP_HASH_TABLE) {
        return NULL;
    }

    size_t hash = hash_lisp_key(key, LISP_HT_BUCKET_COUNT(table));
    struct HashEntry **buckets = (struct HashEntry **)LISP_HT_BUCKETS(table);

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
    if (LISP_TYPE(table) != LISP_HASH_TABLE) {
        return NULL;
    }

    size_t hash = hash_lisp_key(key, LISP_HT_BUCKET_COUNT(table));
    struct HashEntry **buckets = (struct HashEntry **)LISP_HT_BUCKETS(table);

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

    LISP_HT_ENTRY_COUNT(table)
    ++;

    return entry;
}

/* Remove entry from hash table */
int hash_table_remove_entry(LispObject *table, LispObject *key)
{
    if (LISP_TYPE(table) != LISP_HASH_TABLE) {
        return 0;
    }

    size_t hash = hash_lisp_key(key, LISP_HT_BUCKET_COUNT(table));
    struct HashEntry **buckets = (struct HashEntry **)LISP_HT_BUCKETS(table);

    struct HashEntry *entry = buckets[hash];
    struct HashEntry *prev = NULL;

    while (entry) {
        if (hash_keys_equal(entry->key, key)) {
            if (prev) {
                prev->next = entry->next;
            } else {
                buckets[hash] = entry->next;
            }
            LISP_HT_ENTRY_COUNT(table)
            --;
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
    if (LISP_TYPE(table) != LISP_HASH_TABLE) {
        return;
    }

    struct HashEntry **buckets = (struct HashEntry **)LISP_HT_BUCKETS(table);

    for (size_t i = 0; i < LISP_HT_BUCKET_COUNT(table); i++) {
        struct HashEntry *entry = buckets[i];
        while (entry) {
            struct HashEntry *next = entry->next;
            /* GC will handle freeing */
            entry = next;
        }
        buckets[i] = NULL;
    }

    LISP_HT_ENTRY_COUNT(table) = 0;
}
