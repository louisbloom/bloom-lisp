#include "builtins_internal.h"

static LispObject *builtin_make_hash_table(LispObject *args, Environment *env)
{
    (void)env;
    (void)args;
    return lisp_make_hash_table();
}

static LispObject *builtin_hash_ref(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("hash-ref");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-ref requires a hash table");
    }

    LispObject *key_obj = lisp_car(lisp_cdr(args));
    struct HashEntry *entry = hash_table_get_entry(table, key_obj);

    if (entry) {
        return entry->value;
    }

    return NIL; /* Key not found */
}

static LispObject *builtin_hash_set_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_3("hash-set!");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-set! requires a hash table");
    }

    LispObject *key_obj = lisp_car(lisp_cdr(args));
    LispObject *value = lisp_car(lisp_cdr(lisp_cdr(args)));

    hash_table_set_entry(table, key_obj, value);
    return value;
}

static LispObject *builtin_hash_remove_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("hash-remove!");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-remove! requires a hash table");
    }

    LispObject *key_obj = lisp_car(lisp_cdr(args));
    int removed = hash_table_remove_entry(table, key_obj);

    return removed ? LISP_TRUE : NIL;
}

static LispObject *builtin_hash_clear_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-clear!");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-clear! requires a hash table");
    }

    hash_table_clear(table);
    return NIL;
}

static LispObject *builtin_hash_count(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-count");

    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE) {
        return lisp_make_error("hash-count requires a hash table");
    }

    return lisp_make_integer((long long)LISP_HT_ENTRY_COUNT(table));
}

enum hash_iter_mode
{
    HASH_KEYS,
    HASH_VALUES,
    HASH_ENTRIES
};

static LispObject *hash_table_collect(LispObject *table, enum hash_iter_mode mode)
{
    struct HashEntry **buckets = (struct HashEntry **)LISP_HT_BUCKETS(table);
    size_t bucket_count = LISP_HT_BUCKET_COUNT(table);
    LispObject *result = NIL;
    LispObject *tail = NULL;

    for (size_t i = 0; i < bucket_count; i++) {
        for (struct HashEntry *e = buckets[i]; e != NULL; e = e->next) {
            LispObject *item;
            switch (mode) {
            case HASH_KEYS:
                item = e->key;
                break;
            case HASH_VALUES:
                item = e->value;
                break;
            case HASH_ENTRIES:
                item = lisp_make_cons(e->key, e->value);
                break;
            }
            LIST_APPEND(result, tail, item);
        }
    }
    return result;
}

static LispObject *builtin_hash_keys(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-keys");
    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE)
        return lisp_make_error("hash-keys requires a hash table");
    return hash_table_collect(table, HASH_KEYS);
}

static LispObject *builtin_hash_values(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-values");
    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE)
        return lisp_make_error("hash-values requires a hash table");
    return hash_table_collect(table, HASH_VALUES);
}

static LispObject *builtin_hash_entries(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("hash-entries");
    LispObject *table = lisp_car(args);
    if (table->type != LISP_HASH_TABLE)
        return lisp_make_error("hash-entries requires a hash table");
    return hash_table_collect(table, HASH_ENTRIES);
}

void register_hash_tables_builtins(Environment *env)
{
    REGISTER("make-hash-table", builtin_make_hash_table);
    REGISTER("hash-ref", builtin_hash_ref);
    REGISTER("hash-set!", builtin_hash_set_bang);
    REGISTER("hash-remove!", builtin_hash_remove_bang);
    REGISTER("hash-clear!", builtin_hash_clear_bang);
    REGISTER("hash-count", builtin_hash_count);
    REGISTER("hash-keys", builtin_hash_keys);
    REGISTER("hash-values", builtin_hash_values);
    REGISTER("hash-entries", builtin_hash_entries);
}
