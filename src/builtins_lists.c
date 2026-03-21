#include "builtins_internal.h"

/* List operations */
static LispObject *builtin_car(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("car");

    LispObject *arg = lisp_car(args);
    if (arg == NIL) {
        return NIL;
    }

    if (arg->type != LISP_CONS) {
        return lisp_make_error("car requires a list");
    }

    return arg->value.cons.car;
}

static LispObject *builtin_cdr(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("cdr");

    LispObject *arg = lisp_car(args);
    if (arg == NIL) {
        return NIL;
    }

    if (arg->type != LISP_CONS) {
        return lisp_make_error("cdr requires a list");
    }

    return arg->value.cons.cdr;
}

/* c*r combination builtins */
#define DEFINE_CXR(cname, opname, func)                          \
    static LispObject *cname(LispObject *args, Environment *env) \
    {                                                            \
        (void)env;                                               \
        CHECK_ARGS_1(opname);                                    \
        return func(lisp_car(args));                             \
    }
DEFINE_CXR(builtin_caar, "caar", lisp_caar)
DEFINE_CXR(builtin_cadr, "cadr", lisp_cadr)
DEFINE_CXR(builtin_cdar, "cdar", lisp_cdar)
DEFINE_CXR(builtin_cddr, "cddr", lisp_cddr)
DEFINE_CXR(builtin_caddr, "caddr", lisp_caddr)
DEFINE_CXR(builtin_cadddr, "cadddr", lisp_cadddr)
#undef DEFINE_CXR

/* Readable list accessors */
#define DEFINE_ALIAS(cname, target)                              \
    static LispObject *cname(LispObject *args, Environment *env) \
    {                                                            \
        return target(args, env);                                \
    }
DEFINE_ALIAS(builtin_first, builtin_car)
DEFINE_ALIAS(builtin_second, builtin_cadr)
DEFINE_ALIAS(builtin_third, builtin_caddr)
DEFINE_ALIAS(builtin_fourth, builtin_cadddr)
DEFINE_ALIAS(builtin_rest, builtin_cdr)
#undef DEFINE_ALIAS

static LispObject *builtin_set_car_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("set-car!");

    LispObject *pair = lisp_car(args);
    LispObject *value = lisp_car(lisp_cdr(args));

    if (pair == NIL || pair->type != LISP_CONS) {
        return lisp_make_error("set-car! requires a cons cell as first argument");
    }

    pair->value.cons.car = value;
    return value;
}

static LispObject *builtin_set_cdr_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("set-cdr!");

    LispObject *pair = lisp_car(args);
    LispObject *value = lisp_car(lisp_cdr(args));

    if (pair == NIL || pair->type != LISP_CONS) {
        return lisp_make_error("set-cdr! requires a cons cell as first argument");
    }

    pair->value.cons.cdr = value;
    return value;
}

static LispObject *builtin_cons(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("cons");

    LispObject *car = lisp_car(args);
    LispObject *cdr = lisp_car(lisp_cdr(args));

    return lisp_make_cons(car, cdr);
}

static LispObject *builtin_list(LispObject *args, Environment *env)
{
    (void)env;
    return args;
}

static LispObject *builtin_length(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("length");

    LispObject *obj = lisp_car(args);

    switch (obj->type) {
    case LISP_CONS:
    case LISP_NIL:
    {
        /* List length */
        long long count = 0;
        LispObject *lst = obj;
        while (lst != NIL && lst != NULL) {
            count++;
            lst = lst->value.cons.cdr;
        }
        return lisp_make_integer(count);
    }
    case LISP_STRING:
    {
        /* String length (UTF-8 aware) */
        size_t char_count = utf8_strlen(obj->value.string);
        return lisp_make_integer((long long)char_count);
    }
    case LISP_VECTOR:
    {
        /* Vector length */
        return lisp_make_number((double)obj->value.vector.size);
    }
    default:
        return lisp_make_error("length requires a list, string, or vector");
    }
}

static LispObject *builtin_list_ref(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("list-ref");

    LispObject *lst = lisp_car(args);
    LispObject *index_obj = lisp_car(lisp_cdr(args));

    int index_is_integer;
    double index_val = get_numeric_value(index_obj, &index_is_integer);

    if (index_obj->type != LISP_INTEGER && index_obj->type != LISP_NUMBER) {
        return lisp_make_error("list-ref index must be a number");
    }

    long long index = (long long)index_val;
    if (index < 0) {
        return lisp_make_error("list-ref index must be non-negative");
    }

    /* Traverse the list, checking type at each step */
    for (long long i = 0; i < index && lst != NIL && lst != NULL; i++) {
        /* Type check before accessing cons fields */
        if (lst->type != LISP_CONS) {
            return lisp_make_error("list-ref: not a proper list");
        }
        lst = lst->value.cons.cdr;
    }

    if (lst == NIL || lst == NULL) {
        return lisp_make_error("list-ref index out of bounds");
    }

    /* Final type check before accessing car */
    if (lst->type != LISP_CONS) {
        return lisp_make_error("list-ref: not a proper list");
    }

    return lst->value.cons.car;
}

static LispObject *builtin_reverse(LispObject *args, Environment *env)
{
    (void)env;

    if (args == NULL || args == NIL) {
        return lisp_make_error("reverse: expected 1 argument");
    }

    LispObject *lst = lisp_car(args);

    /* Handle empty list */
    if (lst == NIL || lst == NULL) {
        return NIL;
    }

    /* Validate it's a list */
    if (lst->type != LISP_CONS) {
        return lisp_make_error("reverse: argument must be a list");
    }

    /* Build reversed list iteratively */
    LispObject *result = NIL;
    while (lst != NIL && lst != NULL && lst->type == LISP_CONS) {
        result = lisp_make_cons(lisp_car(lst), result);
        lst = lisp_cdr(lst);
    }

    return result;
}

static LispObject *builtin_append(LispObject *args, Environment *env)
{
    (void)env;

    /* No arguments: return empty list */
    if (args == NIL) {
        return NIL;
    }

    /* Build result by concatenating all argument lists */
    LispObject *result = NIL;
    LispObject *result_tail = NIL;

    /* Iterate through each argument */
    LispObject *current_arg = args;
    while (current_arg != NIL) {
        LispObject *list = lisp_car(current_arg);

        /* Each argument must be a list (or NIL) */
        if (list != NIL && list->type != LISP_CONS) {
            return lisp_make_error("append requires list arguments");
        }

        /* Copy elements from this list */
        LispObject *elem = list;
        while (elem != NIL && elem->type == LISP_CONS) {
            LIST_APPEND(result, result_tail, lisp_car(elem));
            elem = lisp_cdr(elem);
        }

        current_arg = lisp_cdr(current_arg);
    }

    return result;
}

/* Helper function to check deep structural equality (used by assoc, equal?, etc.) */
int objects_equal_recursive(LispObject *a, LispObject *b)
{
    /* Fast path: pointer equality */
    if (a == b)
        return 1;

    /* NIL checks */
    if (a == NIL || b == NIL)
        return 0;

    /* Type mismatch */
    if (a->type != b->type)
        return 0;

    switch (a->type) {
    case LISP_NUMBER:
        return a->value.number == b->value.number;

    case LISP_INTEGER:
        return a->value.integer == b->value.integer;

    case LISP_CHAR:
        return a->value.character == b->value.character;

    case LISP_STRING:
        return strcmp(a->value.string, b->value.string) == 0;

    case LISP_SYMBOL:
        /* Symbols are interned, so pointer equality should work, but compare names for safety */
        return strcmp(a->value.symbol->name, b->value.symbol->name) == 0;

    case LISP_BOOLEAN:
        return a->value.boolean == b->value.boolean;

    case LISP_CONS:
        /* Recursive list comparison */
        return objects_equal_recursive(a->value.cons.car, b->value.cons.car) &&
               objects_equal_recursive(a->value.cons.cdr, b->value.cons.cdr);

    case LISP_VECTOR:
        /* Compare vector lengths */
        if (a->value.vector.size != b->value.vector.size) {
            return 0;
        }
        /* Compare each element */
        for (size_t i = 0; i < a->value.vector.size; i++) {
            if (!objects_equal_recursive(a->value.vector.items[i], b->value.vector.items[i])) {
                return 0;
            }
        }
        return 1;

    case LISP_HASH_TABLE:
    {
        /* Compare hash table sizes */
        if (a->value.hash_table.entry_count != b->value.hash_table.entry_count) {
            return 0;
        }
        /* Compare each key-value pair */
        /* Iterate through all buckets in hash table a */
        struct HashEntry **a_buckets = (struct HashEntry **)a->value.hash_table.buckets;
        for (size_t i = 0; i < a->value.hash_table.bucket_count; i++) {
            struct HashEntry *entry = a_buckets[i];
            while (entry != NULL) {
                /* Look up the key in hash table b */
                struct HashEntry *b_entry = hash_table_get_entry(b, entry->key);
                if (b_entry == NULL) {
                    return 0; /* Key doesn't exist in b */
                }
                /* Compare values */
                if (!objects_equal_recursive(entry->value, b_entry->value)) {
                    return 0;
                }
                entry = entry->next;
            }
        }
        return 1;
    }

    default:
        /* For other types (lambdas, builtins, etc.), use pointer equality */
        return 0;
    }
}

static int eq_pointer(LispObject *a, LispObject *b) { return a == b; }

static LispObject *alist_find(LispObject *key, LispObject *alist,
                              int (*eq)(LispObject *, LispObject *),
                              const char *name)
{
    while (alist != NIL && alist != NULL) {
        if (alist->type != LISP_CONS) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s requires an association list", name);
            return lisp_make_error(buf);
        }
        LispObject *pair = lisp_car(alist);
        if (pair != NIL && pair->type == LISP_CONS) {
            if (eq(key, lisp_car(pair)))
                return pair;
        }
        alist = lisp_cdr(alist);
    }
    return NIL;
}

static LispObject *builtin_assoc(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("assoc");
    return alist_find(lisp_car(args), lisp_car(lisp_cdr(args)),
                      objects_equal_recursive, "assoc");
}

static LispObject *builtin_assq(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("assq");
    return alist_find(lisp_car(args), lisp_car(lisp_cdr(args)),
                      eq_pointer, "assq");
}

static LispObject *builtin_assv(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("assv");
    return alist_find(lisp_car(args), lisp_car(lisp_cdr(args)),
                      objects_equal_recursive, "assv");
}

static LispObject *builtin_alist_get(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL || lisp_cdr(args) == NIL) {
        return lisp_make_error("alist-get requires at least 2 arguments");
    }

    LispObject *key = lisp_car(args);
    LispObject *alist = lisp_car(lisp_cdr(args));
    LispObject *default_val = NIL;

    /* Optional third argument is default value */
    if (lisp_cdr(lisp_cdr(args)) != NIL) {
        default_val = lisp_car(lisp_cdr(lisp_cdr(args)));
    }

    /* Iterate through association list */
    while (alist != NIL && alist != NULL) {
        if (alist->type != LISP_CONS) {
            return lisp_make_error("alist-get requires an association list");
        }

        LispObject *pair = lisp_car(alist);
        if (pair != NIL && pair->type == LISP_CONS) {
            LispObject *pair_key = lisp_car(pair);
            if (objects_equal_recursive(key, pair_key)) {
                return lisp_cdr(pair);
            }
        }

        alist = lisp_cdr(alist);
    }

    return default_val;
}

/* List membership */

static LispObject *builtin_member(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("member");

    LispObject *item = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    /* Iterate through list using structural equality */
    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS) {
            return lisp_make_error("member requires a proper list");
        }

        LispObject *elem = lisp_car(list);
        if (objects_equal_recursive(item, elem)) {
            return list;
        }

        list = lisp_cdr(list);
    }

    return NIL;
}

static LispObject *builtin_memq(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("memq");

    LispObject *item = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    /* Iterate through list using pointer equality */
    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS) {
            return lisp_make_error("memq requires a proper list");
        }

        LispObject *elem = lisp_car(list);
        if (item == elem) {
            return list;
        }

        list = lisp_cdr(list);
    }

    return NIL;
}

/* Mapping operations */

/* Apply a unary function (builtin or lambda) to a single item */
static LispObject *apply_unary(LispObject *func, LispObject *item,
                               Environment *env, const char *name)
{
    (void)name;
    return lisp_call_1(func, item, env);
}

static LispObject *builtin_map(LispObject *args, Environment *env)
{
    CHECK_ARGS_2("map");
    LispObject *func = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    if (func->type != LISP_BUILTIN && func->type != LISP_LAMBDA)
        return lisp_make_error("map requires a function as first argument");

    LispObject *result = NIL;
    LispObject *tail = NULL;

    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS)
            return lisp_make_error("map requires a list");

        LispObject *mapped = apply_unary(func, lisp_car(list), env, "map");
        if (mapped->type == LISP_ERROR)
            return mapped;
        LIST_APPEND(result, tail, mapped);
        list = lisp_cdr(list);
    }
    return result;
}

static LispObject *builtin_mapcar(LispObject *args, Environment *env)
{
    return builtin_map(args, env);
}

static LispObject *builtin_filter(LispObject *args, Environment *env)
{
    CHECK_ARGS_2("filter");
    LispObject *func = lisp_car(args);
    LispObject *list = lisp_car(lisp_cdr(args));

    if (func->type != LISP_BUILTIN && func->type != LISP_LAMBDA)
        return lisp_make_error("filter requires a function as first argument");

    LispObject *result = NIL;
    LispObject *tail = NULL;

    while (list != NIL && list != NULL) {
        if (list->type != LISP_CONS)
            return lisp_make_error("filter requires a list");

        LispObject *item = lisp_car(list);
        LispObject *pred = apply_unary(func, item, env, "filter");
        if (pred->type == LISP_ERROR)
            return pred;

        if (pred != NIL &&
            !(pred->type == LISP_BOOLEAN && pred->value.boolean == 0))
            LIST_APPEND(result, tail, item);

        list = lisp_cdr(list);
    }
    return result;
}

static LispObject *builtin_apply(LispObject *args, Environment *env)
{
    CHECK_ARGS_2("apply");

    LispObject *func = lisp_car(args);
    LispObject *func_args = lisp_car(lisp_cdr(args));

    if (func->type != LISP_BUILTIN && func->type != LISP_LAMBDA) {
        return lisp_make_error("apply requires a function as first argument");
    }

    if (func_args != NIL && func_args->type != LISP_CONS) {
        return lisp_make_error("apply requires a list as second argument");
    }

    return lisp_apply(func, func_args, env);
}

void register_lists_builtins(Environment *env)
{
    REGISTER("car", builtin_car);
    REGISTER("cdr", builtin_cdr);
    REGISTER("caar", builtin_caar);
    REGISTER("cadr", builtin_cadr);
    REGISTER("cdar", builtin_cdar);
    REGISTER("cddr", builtin_cddr);
    REGISTER("caddr", builtin_caddr);
    REGISTER("cadddr", builtin_cadddr);
    REGISTER("first", builtin_first);
    REGISTER("second", builtin_second);
    REGISTER("third", builtin_third);
    REGISTER("fourth", builtin_fourth);
    REGISTER("rest", builtin_rest);
    REGISTER("set-car!", builtin_set_car_bang);
    REGISTER("set-cdr!", builtin_set_cdr_bang);
    REGISTER("cons", builtin_cons);
    REGISTER("list", builtin_list);
    REGISTER("length", builtin_length);
    REGISTER("list-ref", builtin_list_ref);
    REGISTER("reverse", builtin_reverse);
    REGISTER("append", builtin_append);
    REGISTER("assoc", builtin_assoc);
    REGISTER("assq", builtin_assq);
    REGISTER("assv", builtin_assv);
    REGISTER("alist-get", builtin_alist_get);
    REGISTER("member", builtin_member);
    REGISTER("memq", builtin_memq);
    REGISTER("map", builtin_map);
    REGISTER("mapcar", builtin_mapcar);
    REGISTER("filter", builtin_filter);
    REGISTER("apply", builtin_apply);
}
