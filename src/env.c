#include "../include/lisp.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

/* Global profiling state */
ProfileState g_profile_state = { 0, NULL, 0 };

/* Get current time in nanoseconds */
uint64_t profile_get_time_ns(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq = { 0 };
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint64_t)(counter.QuadPart * 1000000000ULL / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

/* Find or create profile entry for function */
static ProfileEntry *profile_find_or_create(const char *function_name)
{
    ProfileEntry *entry = g_profile_state.entries;
    while (entry != NULL) {
        if (strcmp(entry->function_name, function_name) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    /* Create new entry */
    entry = GC_malloc(sizeof(ProfileEntry));
    entry->function_name = GC_strdup(function_name);
    entry->call_count = 0;
    entry->total_time_ns = 0;
    entry->next = g_profile_state.entries;
    g_profile_state.entries = entry;
    return entry;
}

/* Record a function call with elapsed time */
void profile_record(const char *function_name, uint64_t elapsed_ns)
{
    ProfileEntry *entry = profile_find_or_create(function_name);
    entry->call_count++;
    entry->total_time_ns += elapsed_ns;
}

/* Reset profiling data */
void profile_reset(void)
{
    g_profile_state.entries = NULL;
    g_profile_state.start_time_ns = 0;
}

static inline size_t hash_symbol(Symbol *sym, size_t bucket_count)
{
    uintptr_t h = (uintptr_t)sym;
    h = (h >> 4) * 2654435761u;
    return h & (bucket_count - 1);
}

static void env_resize(Environment *env)
{
    size_t new_count = env->bucket_count * 2;
    struct Binding **new_buckets = GC_malloc(new_count * sizeof(struct Binding *));
    /* GC_malloc zeroes memory, so all buckets start as NULL */

    for (size_t i = 0; i < env->bucket_count; i++) {
        struct Binding *b = env->buckets[i];
        while (b != NULL) {
            struct Binding *next = b->next;
            size_t idx = hash_symbol(b->symbol, new_count);
            b->next = new_buckets[idx];
            new_buckets[idx] = b;
            b = next;
        }
    }
    env->buckets = new_buckets;
    env->bucket_count = new_count;
}

Environment *env_create(Environment *parent)
{
    Environment *env = GC_malloc(sizeof(Environment));
    /* GC_malloc zeroes memory, so inline_buckets are all NULL */
    env->buckets = env->inline_buckets;
    env->bucket_count = ENV_INLINE_BUCKETS;
    env->binding_count = 0;
    env->parent = parent;
    env->call_stack = NULL;
    env->handler_stack = NULL;
    return env;
}

void env_define(Environment *env, Symbol *sym, LispObject *value, Symbol *package)
{
    size_t idx = hash_symbol(sym, env->bucket_count);
    /* Check if binding already exists (pointer comparison) */
    struct Binding *binding = env->buckets[idx];
    while (binding != NULL) {
        if (binding->symbol == sym) {
            binding->value = value;
            return;
        }
        binding = binding->next;
    }

    /* Resize if load factor exceeded (> 3/4) */
    if (env->binding_count * 4 >= env->bucket_count * 3) {
        env_resize(env);
        idx = hash_symbol(sym, env->bucket_count);
    }

    /* Create new binding */
    binding = GC_malloc(sizeof(struct Binding));
    binding->symbol = sym;
    binding->value = value;
    binding->package = package;
    binding->next = env->buckets[idx];
    env->buckets[idx] = binding;
    env->binding_count++;
}

LispObject *env_lookup(Environment *env, Symbol *sym)
{
    while (env != NULL) {
        size_t idx = hash_symbol(sym, env->bucket_count);
        struct Binding *binding = env->buckets[idx];
        while (binding != NULL) {
            if (binding->symbol == sym) {
                return binding->value;
            }
            binding = binding->next;
        }
        env = env->parent;
    }
    return NULL;
}

LispObject *env_lookup_in_package(Environment *env, Symbol *sym, Symbol *package)
{
    while (env != NULL) {
        size_t idx = hash_symbol(sym, env->bucket_count);
        struct Binding *binding = env->buckets[idx];
        while (binding != NULL) {
            if (binding->symbol == sym && binding->package == package) {
                return binding->value;
            }
            binding = binding->next;
        }
        env = env->parent;
    }
    return NULL;
}

Symbol *env_current_package(Environment *env)
{
    LispObject *val = env_lookup(env, sym_star_package_star->value.symbol);
    if (val != NULL && val->type == LISP_SYMBOL) {
        return val->value.symbol;
    }
    return pkg_core;
}

int env_set(Environment *env, Symbol *sym, LispObject *value)
{
    /* Look for binding in current or parent environments */
    while (env != NULL) {
        size_t idx = hash_symbol(sym, env->bucket_count);
        struct Binding *binding = env->buckets[idx];
        while (binding != NULL) {
            if (binding->symbol == sym) {
                binding->value = value;
                return 1; /* Successfully updated */
            }
            binding = binding->next;
        }
        env = env->parent;
    }
    return 0; /* Variable not found */
}

void env_free(Environment *env)
{
    /* GC handles cleanup automatically */
    /* We don't need to free individual bindings or the environment */
    (void)env; /* Suppress unused parameter warning */
}

/* Call stack frame free-list (LIFO reuse, avoids GC_malloc per call) */
static CallStackFrame *frame_free_list = NULL;

/* Call stack functions */
void push_call_frame(Environment *env, const char *function_name)
{
    CallStackFrame *frame;
    if (frame_free_list != NULL) {
        frame = frame_free_list;
        frame_free_list = frame->parent;
    } else {
        frame = GC_malloc(sizeof(CallStackFrame));
    }
    frame->function_name = (char *)function_name;
    frame->parent = env->call_stack;
    frame->entry_time_ns = 0;

    if (g_profile_state.enabled) {
        frame->entry_time_ns = profile_get_time_ns();
    }

    env->call_stack = frame;
}

void pop_call_frame(Environment *env)
{
    if (env->call_stack != NULL) {
        if (g_profile_state.enabled && env->call_stack->entry_time_ns != 0) {
            uint64_t elapsed = profile_get_time_ns() - env->call_stack->entry_time_ns;
            profile_record(env->call_stack->function_name, elapsed);
        }
        CallStackFrame *frame = env->call_stack;
        env->call_stack = frame->parent;
        frame->parent = frame_free_list;
        frame_free_list = frame;
    }
}

LispObject *capture_call_stack(Environment *env)
{
    LispObject *stack = NIL;
    CallStackFrame *frame = env->call_stack;
    int depth = 0;
    const int MAX_STACK_DEPTH = 20;

    /* Traverse and collect frames */
    while (frame != NULL && depth < MAX_STACK_DEPTH) {
        stack = lisp_make_cons(lisp_make_string(frame->function_name), stack);
        frame = frame->parent;
        depth++;
    }

    /* Reverse the list to get chronological order */
    LispObject *reversed = NIL;
    LispObject *current = stack;
    while (current != NIL && current->type == LISP_CONS) {
        reversed = lisp_make_cons(lisp_car(current), reversed);
        current = lisp_cdr(current);
    }

    return reversed;
}
