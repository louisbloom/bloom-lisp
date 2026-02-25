#include "builtins_internal.h"

#ifdef _WIN32
#include <windows.h>
#endif

static LispObject *builtin_current_time_ms(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

#ifdef _WIN32
    /* Windows: use GetTickCount64 for millisecond precision */
    return lisp_make_integer((long long)GetTickCount64());
#else
    /* POSIX: use clock_gettime with CLOCK_MONOTONIC */
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        long long ms = (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        return lisp_make_integer(ms);
    }
    /* Fallback to time() if clock_gettime fails (unlikely) */
    return lisp_make_integer((long long)time(NULL) * 1000);
#endif
}

/* Profiling builtins */
static LispObject *builtin_profile_start(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;
    profile_reset();
    g_profile_state.enabled = 1;
    g_profile_state.start_time_ns = profile_get_time_ns();
    return LISP_TRUE;
}

static LispObject *builtin_profile_stop(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;
    g_profile_state.enabled = 0;
    return LISP_TRUE;
}

static LispObject *builtin_profile_report(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    /* Build alist of (function-name call-count total-ms) sorted by total time desc */
    /* First, count entries and collect into array for sorting */
    int count = 0;
    for (ProfileEntry *e = g_profile_state.entries; e != NULL; e = e->next) {
        count++;
    }

    if (count == 0) {
        return NIL;
    }

    /* Create array of pointers for sorting */
    ProfileEntry **arr = GC_malloc(sizeof(ProfileEntry *) * count);
    int i = 0;
    for (ProfileEntry *e = g_profile_state.entries; e != NULL; e = e->next) {
        arr[i++] = e;
    }

    /* Sort by total_time_ns descending (simple insertion sort, good enough for profiles) */
    for (int j = 1; j < count; j++) {
        ProfileEntry *key = arr[j];
        int k = j - 1;
        while (k >= 0 && arr[k]->total_time_ns < key->total_time_ns) {
            arr[k + 1] = arr[k];
            k--;
        }
        arr[k + 1] = key;
    }

    /* Build result list (reversed order to end up sorted) */
    LispObject *result = NIL;
    for (int j = count - 1; j >= 0; j--) {
        ProfileEntry *e = arr[j];
        /* Convert ns to ms (double for precision) */
        double total_ms = (double)e->total_time_ns / 1000000.0;

        /* Build entry: (name call-count total-ms) */
        LispObject *entry = lisp_make_cons(lisp_make_string(e->function_name),
                                           lisp_make_cons(lisp_make_integer((long long)e->call_count),
                                                          lisp_make_cons(lisp_make_number(total_ms), NIL)));
        result = lisp_make_cons(entry, result);
    }

    return result;
}

static LispObject *builtin_profile_reset(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;
    profile_reset();
    return LISP_TRUE;
}

void register_time_profiling_builtins(Environment *env)
{
    REGISTER("current-time-ms", builtin_current_time_ms);
    REGISTER("profile-start", builtin_profile_start);
    REGISTER("profile-stop", builtin_profile_stop);
    REGISTER("profile-report", builtin_profile_report);
    REGISTER("profile-reset", builtin_profile_reset);
}
