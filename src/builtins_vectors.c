#include "builtins_internal.h"

static LispObject *builtin_make_vector(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL) {
        return lisp_make_error("make-vector requires at least 1 argument");
    }
    LispObject *size_obj = lisp_car(args);
    if (size_obj->type != LISP_NUMBER && size_obj->type != LISP_INTEGER) {
        return lisp_make_error("make-vector size must be a number");
    }
    size_t size = (size_t)(size_obj->type == LISP_INTEGER ? size_obj->value.integer : size_obj->value.number);

    LispObject *vec = lisp_make_vector(size);

    /* If initial value is provided, set all elements to it */
    if (lisp_cdr(args) != NIL) {
        LispObject *init_val = lisp_car(lisp_cdr(args));
        for (size_t i = 0; i < size; i++) {
            LISP_VECTOR_ITEMS(vec)
            [i] = init_val;
        }
        /* Set the size to match the capacity when initializing */
        LISP_VECTOR_SIZE(vec) = size;
    }

    return vec;
}

static LispObject *builtin_vector_ref(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("vector-ref");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-ref requires a vector");
    }
    LispObject *idx_obj = lisp_car(lisp_cdr(args));
    if (idx_obj->type != LISP_NUMBER && idx_obj->type != LISP_INTEGER) {
        return lisp_make_error("vector-ref index must be a number");
    }
    size_t idx = (size_t)(idx_obj->type == LISP_INTEGER ? idx_obj->value.integer : idx_obj->value.number);
    if (idx >= LISP_VECTOR_SIZE(vec_obj)) {
        return lisp_make_error("vector-ref: index out of bounds");
    }
    return LISP_VECTOR_ITEMS(vec_obj)[idx];
}

static LispObject *builtin_vector_set_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_3("vector-set!");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-set! requires a vector");
    }
    LispObject *idx_obj = lisp_car(lisp_cdr(args));
    if (idx_obj->type != LISP_NUMBER && idx_obj->type != LISP_INTEGER) {
        return lisp_make_error("vector-set! index must be a number");
    }
    size_t idx = (size_t)(idx_obj->type == LISP_INTEGER ? idx_obj->value.integer : idx_obj->value.number);
    if (idx >= LISP_VECTOR_SIZE(vec_obj)) {
        LISP_VECTOR_SIZE(vec_obj) = idx + 1;
        /* Expand capacity if needed */
        if (LISP_VECTOR_SIZE(vec_obj) > LISP_VECTOR_CAPACITY(vec_obj)) {
            size_t new_capacity = LISP_VECTOR_CAPACITY(vec_obj);
            while (new_capacity < LISP_VECTOR_SIZE(vec_obj)) {
                new_capacity *= 2;
            }
            LispObject **new_items = GC_malloc(sizeof(LispObject *) * new_capacity);
            for (size_t i = 0; i < LISP_VECTOR_SIZE(vec_obj) - 1; i++) {
                new_items[i] = LISP_VECTOR_ITEMS(vec_obj)[i];
            }
            for (size_t i = LISP_VECTOR_SIZE(vec_obj) - 1; i < new_capacity; i++) {
                new_items[i] = NIL;
            }
            LISP_VECTOR_ITEMS(vec_obj) = new_items;
            LISP_VECTOR_CAPACITY(vec_obj) = new_capacity;
        }
    }
    LispObject *value = lisp_car(lisp_cdr(lisp_cdr(args)));
    LISP_VECTOR_ITEMS(vec_obj)
    [idx] = value;
    return value;
}

static LispObject *builtin_vector_push_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_2("vector-push!");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-push! requires a vector");
    }
    LispObject *value = lisp_car(lisp_cdr(args));
    /* Check if we need to expand */
    if (LISP_VECTOR_SIZE(vec_obj) >= LISP_VECTOR_CAPACITY(vec_obj)) {
        size_t new_capacity = LISP_VECTOR_CAPACITY(vec_obj) * 2;
        LispObject **new_items = GC_malloc(sizeof(LispObject *) * new_capacity);
        for (size_t i = 0; i < LISP_VECTOR_SIZE(vec_obj); i++) {
            new_items[i] = LISP_VECTOR_ITEMS(vec_obj)[i];
        }
        for (size_t i = LISP_VECTOR_SIZE(vec_obj); i < new_capacity; i++) {
            new_items[i] = NIL;
        }
        LISP_VECTOR_ITEMS(vec_obj) = new_items;
        LISP_VECTOR_CAPACITY(vec_obj) = new_capacity;
    }
    LISP_VECTOR_ITEMS(vec_obj)
    [LISP_VECTOR_SIZE(vec_obj)] = value;
    LISP_VECTOR_SIZE(vec_obj)
    ++;
    return value;
}

static LispObject *builtin_vector_pop_bang(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("vector-pop!");
    LispObject *vec_obj = lisp_car(args);
    if (vec_obj->type != LISP_VECTOR) {
        return lisp_make_error("vector-pop! requires a vector");
    }
    if (LISP_VECTOR_SIZE(vec_obj) == 0) {
        return lisp_make_error("vector-pop!: cannot pop from empty vector");
    }
    LISP_VECTOR_SIZE(vec_obj)
    --;
    return LISP_VECTOR_ITEMS(vec_obj)[LISP_VECTOR_SIZE(vec_obj)];
}

void register_vectors_builtins(Environment *env)
{
    REGISTER("make-vector", builtin_make_vector);
    REGISTER("vector-ref", builtin_vector_ref);
    REGISTER("vector-set!", builtin_vector_set_bang);
    REGISTER("vector-push!", builtin_vector_push_bang);
    REGISTER("vector-pop!", builtin_vector_pop_bang);
}
