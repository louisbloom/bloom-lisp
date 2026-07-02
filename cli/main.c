#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "file_utils.h"
#include "lisp.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_BOBA
#include "colors.h"
#include "repl_app.h"
#include <boba/ansi_sequences.h>
#include <boba/cmd.h>
#include <boba/runtime.h>
#include <ditty/highlight.h>
#endif

#include "ditty_version.h"
#ifndef DITTY_VERSION
#define DITTY_VERSION "unknown"
#endif

/* Boehm GC manages most allocations, and its finalizers (e.g. the one that frees
 * a compiled regex's pcre2_code) are not guaranteed to run at process exit.
 * LeakSanitizer also cannot see into the GC heap to know those blocks are still
 * referenced, so it reports false positives. Disable leak detection; ASan and
 * UBSan still catch real memory errors. */
const char *__asan_default_options(void)
{
    return "detect_leaks=0";
}

#ifdef HAVE_BOBA

/* Global state */
static Environment *g_env = NULL;
static ReplAppModel *g_app = NULL;
static TuiRuntime *g_runtime = NULL;
static FlareLexer *g_lexer = NULL; /* reused across highlight calls */

/* ANSI color buffers */
static char color_prompt[32];
static char color_error[32];
static char color_info[32];
static char color_nil[32];
static char color_number[32];
static char string_color[32];
static char color_symbol[32];
static char color_function[32];
static char color_result[32];

static void init_colors(void)
{
    ansi_format_fg_color_rgb(color_prompt, sizeof(color_prompt),
                             COLOR_PROMPT_R, COLOR_PROMPT_G, COLOR_PROMPT_B);
    ansi_format_fg_color_rgb(color_error, sizeof(color_error),
                             COLOR_ERROR_R, COLOR_ERROR_G, COLOR_ERROR_B);
    ansi_format_fg_color_rgb(color_info, sizeof(color_info),
                             COLOR_INFO_R, COLOR_INFO_G, COLOR_INFO_B);
    ansi_format_fg_color_rgb(color_nil, sizeof(color_nil),
                             COLOR_NIL_R, COLOR_NIL_G, COLOR_NIL_B);
    ansi_format_fg_color_rgb(color_number, sizeof(color_number),
                             COLOR_NUMBER_R, COLOR_NUMBER_G, COLOR_NUMBER_B);
    ansi_format_fg_color_rgb(string_color, sizeof(string_color),
                             COLOR_STRING_R, COLOR_STRING_G, COLOR_STRING_B);
    ansi_format_fg_color_rgb(color_symbol, sizeof(color_symbol),
                             COLOR_SYMBOL_R, COLOR_SYMBOL_G, COLOR_SYMBOL_B);
    ansi_format_fg_color_rgb(color_function, sizeof(color_function),
                             COLOR_FUNCTION_R, COLOR_FUNCTION_G, COLOR_FUNCTION_B);
    ansi_format_fg_color_rgb(color_result, sizeof(color_result),
                             COLOR_RESULT_R, COLOR_RESULT_G, COLOR_RESULT_B);
}

static const char *color_for_type(LispType type)
{
    switch (type) {
    case LISP_NIL:
        return color_nil;
    case LISP_NUMBER:
    case LISP_INTEGER:
    case LISP_CHAR:
        return color_number;
    case LISP_STRING:
        return string_color;
    case LISP_SYMBOL:
        return color_symbol;
    case LISP_LAMBDA:
    case LISP_MACRO:
    case LISP_BUILTIN:
        return color_function;
    default:
        return color_result;
    }
}

/* --- Flare highlight callback for textinput --- */

static char *highlight_lisp(const char *text, size_t len, void *userdata)
{
    (void)userdata;
    FlareResult r = flare_highlight(text, len, g_lexer, NULL, NULL);
    /* flare_highlight returns malloc'd data; textinput will free it.
     * If highlighting fails, fall back to plain text. */
    if (r.data)
        return r.data;
    char *fallback = malloc(len + 1);
    memcpy(fallback, text, len);
    fallback[len] = '\0';
    return fallback;
}

/* --- Completion --- */

static LispCompleteContext detect_context(const char *buffer, int cursor_pos)
{
    int i = cursor_pos - 1;

    while (i >= 0 && buffer[i] != ' ' && buffer[i] != '\t' && buffer[i] != '(' && buffer[i] != ')' &&
           buffer[i] != '\'' && buffer[i] != '`') {
        i--;
    }

    while (i >= 0 && (buffer[i] == ' ' || buffer[i] == '\t')) {
        i--;
    }

    if (i >= 0 && buffer[i] == '(') {
        return LISP_COMPLETE_CALLABLE;
    }

    int j = i;
    if (j >= 0) {
        int func_end = j + 1;
        while (j >= 0 && buffer[j] != ' ' && buffer[j] != '\t' && buffer[j] != '(') {
            j--;
        }
        int func_start = j + 1;
        int func_len = func_end - func_start;

        if (func_len == 4 && strncmp(buffer + func_start, "set!", 4) == 0) {
            while (j >= 0 && (buffer[j] == ' ' || buffer[j] == '\t')) {
                j--;
            }
            if (j >= 0 && buffer[j] == '(') {
                return LISP_COMPLETE_VARIABLE;
            }
        }
    }

    return LISP_COMPLETE_ALL;
}

/* --- Completeness check for Enter interception --- */

static int is_form_complete(const char *text)
{
    const char *ptr = text;
    LispObject *expr = lisp_read(&ptr);
    if (expr == NULL)
        return 1;
    if (LISP_TYPE(expr) == LISP_ERROR &&
        LISP_ERROR_TYPE(expr) == sym_unclosed_input)
        return 0;
    return 1;
}

/* --- Auto-indentation helper --- */

static int count_unclosed_parens(const char *text)
{
    int depth = 0;
    int in_string = 0;
    int in_comment = 0;

    for (const char *p = text; *p; p++) {
        if (in_comment) {
            if (p[0] == '\n')
                in_comment = 0;
            continue;
        }
        if (in_string) {
            if (*p == '\\' && p[1])
                p++;
            else if (*p == '"')
                in_string = 0;
            continue;
        }
        if (*p == ';') {
            in_comment = 1;
            continue;
        }
        if (*p == '"') {
            in_string = 1;
            continue;
        }
        if (*p == '(')
            depth++;
        else if (*p == ')')
            depth--;
    }
    return depth < 0 ? 0 : depth;
}

static int compute_auto_indent(const char *text)
{
    return count_unclosed_parens(text) * 2;
}

/* Ctrl-C handler: abort current edit, start fresh prompt */
static void handle_break(void)
{
    tui_textinput_clear(g_app->textinput);
    tui_runtime_finish_inline(g_runtime);
}

#endif /* HAVE_BOBA */

/* --- Non-interactive helpers --- */

static LispObject *argv_to_list(int start, int end, char **argv)
{
    LispObject *result = NIL;
    for (int i = end - 1; i >= start; i--) {
        LispObject *str = lisp_make_string(argv[i]);
        result = lisp_make_cons(str, result);
    }
    return result;
}

static void print_help(void)
{
    printf("Bloom Lisp Interpreter v%s\n", DITTY_VERSION);
    printf("\n");
    printf("Usage:\n");
    printf("  ditty                          Start interactive REPL\n");
    printf("  ditty -e \"CODE\"                Execute CODE and exit\n");
    printf("  ditty FILE [FILE...]           Execute FILE(s) and exit\n");
    printf("  ditty FILE -- [ARG...]         Run FILE with script arguments\n");
    printf("  ditty -h, --help               Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  ditty                            # Start REPL\n");
    printf("  ditty -e \"(+ 1 2 3)\"             # Execute code\n");
    printf("  ditty script.lisp                # Run file\n");
    printf("  ditty script.lisp -- -i foo.txt  # Run with args in *command-line-args*\n");
    printf("  ditty -e \"(define x 10) (* x 5)\" # Multiple expressions\n");
    printf("\n");
    printf("REPL Commands:\n");
    printf("  /quit             Exit the REPL\n");
    printf("  /load <filename>  Load and execute a Lisp file\n");
    printf("\n");
    printf("See LANGUAGE_REFERENCE.md for complete language documentation.\n");
}

#ifdef HAVE_BOBA

/* --- REPL command handler --- */

static int handle_command(const char *input, Environment *env)
{
    while (*input == ' ' || *input == '\t')
        input++;

    if (strncmp(input, "/quit", 5) == 0) {
        return 1;
    }

    if (strncmp(input, "/load", 5) == 0) {
        const char *filename = input + 5;
        while (*filename == ' ' || *filename == '\t')
            filename++;

        if (*filename == '\0') {
            printf("%sERROR: /load requires a filename%s\n",
                   color_error, SGR_RESET);
            return 0;
        }

        char *fname = GC_strdup(filename);
        LispObject *result = lisp_load_file(fname, env);

        if (LISP_TYPE(result) == LISP_ERROR) {
            char *err_str = lisp_print(result);
            printf("%sERROR: %s%s\n", color_error, err_str, SGR_RESET);
        } else {
            char *output = lisp_print(result);
            const char *clr = color_for_type(LISP_TYPE(result));
            printf("%s%s%s\n", clr, output, SGR_RESET);
        }

        return 0;
    }

    return -1;
}

/* --- Runtime command handler --- */

static void handle_line_submit(char *line)
{
    /* The textinput's line_submit() already cleared the buffer and
     * passed us the submitted text. Enter only reaches submit when
     * the form is complete (repl_app_update intercepts incomplete). */
    const char *full_text = line ? line : "";

    /* Handle /quit / /load */
    if (full_text[0] == '/') {
        int cmd_result = handle_command(full_text, g_env);
        if (cmd_result == 1) {
            tui_runtime_schedule(g_runtime, tui_cmd_quit());
            return;
        } else if (cmd_result == 0) {
            tui_runtime_flush(g_runtime);
            return;
        }
    }

    /* Parse the submitted text */
    const char *input_ptr = full_text;
    LispObject *expr = lisp_read(&input_ptr);

    if (expr == NULL || LISP_TYPE(expr) == LISP_ERROR) {
        tui_runtime_flush(g_runtime);
        return;
    }

    /* Complete form — evaluate it.
     * Use tui_runtime_finish_inline to move cursor past all rendered
     * content (even if cursor was on an earlier line), then print
     * output directly. The event loop's next flush renders the fresh
     * prompt below the output. */
    tui_runtime_finish_inline(g_runtime);

    /* Add to history */
    tui_textinput_history_add(g_app->textinput, full_text);

    LispObject *eval_result = lisp_eval(expr, g_env);

    /* Display result */
    if (LISP_TYPE(eval_result) == LISP_ERROR && !LISP_ERROR_CAUGHT(eval_result)) {
        char *err_str = lisp_print(eval_result);
        printf("%sERROR: %s%s\n", color_error, err_str, SGR_RESET);
    } else {
        char *output = lisp_print(eval_result);
        const char *clr = color_for_type(LISP_TYPE(eval_result));
        printf("%s%s%s\n", clr, output, SGR_RESET);
    }
    fflush(stdout);
}

static void handle_tab_complete(TuiCmd *cmd)
{
    char *prefix = cmd->payload.tab_complete.prefix;
    int word_start = cmd->payload.tab_complete.word_start;

    const char *buffer = tui_textinput_text(g_app->textinput);
    int cursor_pos = (int)tui_textinput_cursor(g_app->textinput);

    LispCompleteContext ctx = detect_context(buffer, cursor_pos);
    char **completions = lisp_get_completions(g_env, prefix, ctx);

    if (completions && completions[0]) {
        int count = 0;
        while (completions[count])
            count++;

        if (count == 1) {
            tui_textinput_insert_completion(g_app->textinput, word_start, completions[0]);
        } else {
            const char *first = completions[0];
            int common_len = (int)strlen(first);
            for (int c = 1; c < count; c++) {
                int j = 0;
                while (j < common_len && completions[c][j] == first[j])
                    j++;
                common_len = j;
            }

            int prefix_len = (int)strlen(prefix);
            if (common_len > prefix_len) {
                char *common = malloc(common_len + 1);
                if (common) {
                    memcpy(common, first, common_len);
                    common[common_len] = '\0';
                    tui_textinput_insert_completion(g_app->textinput, word_start, common);
                    free(common);
                }
            } else {
                /* Display tabular list */
                int max_len = 0;
                for (int c = 0; c < count; c++) {
                    int len = (int)strlen(completions[c]);
                    if (len > max_len)
                        max_len = len;
                }
                int col_width = max_len + 2;
                int term_cols = tui_runtime_get_width(g_runtime);
                int cols = term_cols / col_width;
                if (cols < 1)
                    cols = 1;

                /* Print completions directly to terminal */
                tui_runtime_stop(g_runtime);
                printf("\n");
                for (int c = 0; c < count; c++) {
                    int last_in_row = ((c + 1) % cols == 0) || (c == count - 1);
                    if (last_in_row)
                        printf("%s\n", completions[c]);
                    else
                        printf("%-*s", col_width, completions[c]);
                }
                tui_runtime_start(g_runtime);
            }
        }

        for (int c = 0; c < count; c++)
            free(completions[c]);
        free(completions);
    }
}

static void handle_app_cmd(TuiCmd *cmd, void *user_data)
{
    (void)user_data;

    if (cmd->type == TUI_CMD_LINE_SUBMIT) {
        handle_line_submit(cmd->payload.line);
    } else if (cmd->type == TUI_CMD_TAB_COMPLETE) {
        handle_tab_complete(cmd);
    }

    tui_cmd_free(cmd);
}

/* --- Cleanup --- */

static void cleanup(void)
{
    g_app = NULL;

    if (g_lexer) {
        flare_lexer_free(g_lexer);
        g_lexer = NULL;
    }

    if (g_runtime) {
        tui_runtime_free(g_runtime);
        g_runtime = NULL;
    }
}

/* --- Interactive REPL --- */

static void run_interactive_repl(Environment *env)
{
    init_colors();

    /* Create flare lexer for syntax highlighting (reused across keystrokes) */
    g_lexer = flare_lexer_ditty(env);

    ReplAppConfig app_config = {
        .terminal_width = 80,
        .terminal_height = 24,
    };
    TuiRuntimeConfig runtime_config = {
        .raw_mode = 1,
        .output = stdout,
        .cmd_handler = handle_app_cmd,
        .cmd_handler_data = NULL,
    };
    g_runtime = tui_runtime_create((TuiComponent *)repl_app_component(), &app_config, &runtime_config);
    if (!g_runtime) {
        fprintf(stderr, "ERROR: Failed to create TUI runtime\n");
        return;
    }
    g_app = (ReplAppModel *)tui_runtime_model(g_runtime);

    /* Set prompt style */
    TuiStyle prompt_style = tui_style_foreground(
        tui_style_new(),
        tui_color_rgb(COLOR_PROMPT_R, COLOR_PROMPT_G, COLOR_PROMPT_B));
    tui_textinput_set_focused_prompt_style(g_app->textinput, prompt_style);
    tui_textinput_set_blurred_prompt_style(g_app->textinput, prompt_style);

    /* Wire flare syntax highlighting into the textinput */
    tui_textinput_set_text_renderer(g_app->textinput, highlight_lisp, NULL);

    /* Wire completeness check for Enter interception */
    g_app->is_complete = is_form_complete;
    g_app->compute_indent = compute_auto_indent;
    g_app->on_break = handle_break;

    atexit(cleanup);

    /* Welcome message — printed before entering inline mode */
    printf("%sDitty Lisp Interpreter v%s\n"
           "Type expressions to evaluate, /quit to exit, /load <file> to load a file\n"
           "Tab for completion, Up/Down for history%s\n\n",
           color_info, DITTY_VERSION, SGR_RESET);

    tui_runtime_run(g_runtime);
}

#endif /* HAVE_BOBA */

/* --- Main --- */

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    /* Handle help flag */
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)) {
        print_help();
        return 0;
    }

    /* Initialize interpreter */
    Environment *env = lisp_init();
    if (!env) {
        fprintf(stderr, "ERROR: Failed to initialize Lisp interpreter\n");
        return 1;
    }
#ifdef HAVE_BOBA
    g_env = env;
#endif

    env_define(env, LISP_SYM_VAL(lisp_intern("*command-line-args*")), NIL, pkg_user);

    /* Handle -e/--eval/-c flag */
    if (argc > 2 && (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "--eval") == 0 || strcmp(argv[1], "-c") == 0)) {
        const char *code = argv[2];
        const char *input = code;

        while (*input) {
            const char *parse_start = input;
            LispObject *expr = lisp_read(&input);

            if (expr == NULL || input == parse_start)
                break;

            if (LISP_TYPE(expr) == LISP_ERROR) {
                char *err_str = lisp_print(expr);
                fprintf(stderr, "ERROR: %s\n", err_str);
                lisp_cleanup();
                return 1;
            }

            LispObject *result = lisp_eval(expr, env);

            if (LISP_TYPE(result) == LISP_ERROR && !LISP_ERROR_CAUGHT(result)) {
                char *err_str = lisp_print(result);
                fprintf(stderr, "ERROR: %s\n", err_str);
                lisp_cleanup();
                return 1;
            }

            char *output = lisp_print(result);
            printf("%s\n", output);
        }

        lisp_cleanup();
        return 0;
    }

    /* File execution mode */
    if (argc > 1) {
        int separator_pos = -1;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--") == 0) {
                separator_pos = i;
                break;
            }
        }

        int file_end = (separator_pos > 0) ? separator_pos : argc;

        if (separator_pos > 0) {
            LispObject *args_list = argv_to_list(separator_pos + 1, argc, argv);
            env_set(env, LISP_SYM_VAL(lisp_intern("*command-line-args*")), args_list);
        }

        for (int i = 1; i < file_end; i++) {
            char resolved[4096];
            const char *path = file_resolve(argv[i], resolved, sizeof(resolved));
            FILE *file = file_open(path, "rb");
            if (file == NULL) {
                fprintf(stderr, "ERROR: Cannot open file: %s\n", argv[i]);
                return 1;
            }

            fseek(file, 0, SEEK_END);
            long size = ftell(file);
            fseek(file, 0, SEEK_SET);

            char *buffer = GC_malloc(size + 1);
            size_t actual_read = fread(buffer, 1, size, file);
            buffer[actual_read] = '\0';
            fclose(file);

            const char *input = buffer;

            while (*input) {
                const char *parse_start = input;
                LispObject *expr = lisp_read(&input);

                if (expr == NULL || input == parse_start)
                    break;

                if (LISP_TYPE(expr) == LISP_ERROR) {
                    char *err_str = lisp_print(expr);
                    fprintf(stderr, "ERROR in %s: %s\n", argv[i], err_str);
                    return 1;
                }

                LispObject *result = lisp_eval(expr, env);

                if (LISP_TYPE(result) == LISP_ERROR && !LISP_ERROR_CAUGHT(result)) {
                    char *err_str = lisp_print(result);
                    fprintf(stderr, "ERROR in %s: %s\n", argv[i], err_str);
                    return 1;
                }
            }
        }

        lisp_cleanup();
        return 0;
    }

#ifdef HAVE_BOBA
    /* Interactive REPL */
    run_interactive_repl(env);

    env_free(env);
    lisp_cleanup();

    return 0;
#else
    /* Line-mode REPL fallback (no boba required) */
    printf("Ditty Lisp Interpreter v%s\n", DITTY_VERSION);
    printf("Type expressions to evaluate, /quit to exit\n\n");

    char line[8192];
    char expr_buf[8192] = { 0 };
    int expr_len = 0;

    while (1) {
        const char *prompt = expr_len > 0 ? "... " : ">>> ";
        printf("%s", prompt);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        /* Strip trailing newline */
        size_t line_len = strlen(line);
        while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r'))
            line[--line_len] = '\0';

        /* Skip blank lines when not accumulating */
        if (expr_len == 0) {
            int blank = 1;
            for (size_t i = 0; i < line_len; i++) {
                if (line[i] != ' ' && line[i] != '\t') {
                    blank = 0;
                    break;
                }
            }
            if (blank)
                continue;
        }

        /* Handle /quit */
        if (expr_len == 0 && strncmp(line, "/quit", 5) == 0)
            break;

        /* Accumulate */
        if (expr_len > 0 && expr_len < (int)sizeof(expr_buf) - 2)
            expr_buf[expr_len++] = ' ';
        if (expr_len + (int)line_len < (int)sizeof(expr_buf) - 1) {
            strcpy(expr_buf + expr_len, line);
            expr_len += line_len;
        }

        /* Try to parse */
        const char *input_ptr = expr_buf;
        LispObject *expr = lisp_read(&input_ptr);

        if (expr != NULL && LISP_TYPE(expr) == LISP_ERROR &&
            LISP_ERROR_TYPE(expr) == sym_unclosed_input) {
            /* Incomplete — keep reading */
            continue;
        }

        if (expr == NULL) {
            expr_len = 0;
            expr_buf[0] = '\0';
            continue;
        }

        if (LISP_TYPE(expr) == LISP_ERROR) {
            char *err_str = lisp_print(expr);
            printf("ERROR: %s\n", err_str);
            expr_len = 0;
            expr_buf[0] = '\0';
            continue;
        }

        /* Evaluate */
        LispObject *result = lisp_eval(expr, env);

        if (LISP_TYPE(result) == LISP_ERROR && !LISP_ERROR_CAUGHT(result)) {
            char *err_str = lisp_print(result);
            printf("ERROR: %s\n", err_str);
        } else {
            char *output = lisp_print(result);
            printf("%s\n", output);
        }

        expr_len = 0;
        expr_buf[0] = '\0';
    }

    lisp_cleanup();
    return 0;
#endif
}
