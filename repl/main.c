#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "file_utils.h"
#include "lisp.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_BLOOM_BOBA
#include "colors.h"
#include "repl_app.h"
#include <bloom-boba/ansi_sequences.h>
#include <bloom-boba/cmd.h>
#include <bloom-boba/runtime.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/* Version information - fallback if not defined by autoconf */
#ifndef BLOOM_LISP_VERSION
#define BLOOM_LISP_VERSION "unknown"
#endif

#ifdef HAVE_BLOOM_BOBA

/* Global state */
static Environment *g_env = NULL;
static ReplAppModel *g_app = NULL;
static TuiRuntime *g_runtime = NULL;

/* ANSI color buffers */
static char color_prompt[32];
static char color_divider[32];
static char color_error[32];
static char color_info[32];
static char color_nil[32];
static char color_number[32];
static char color_string[32];
static char color_symbol[32];
static char color_function[32];
static char color_result[32];

static void init_colors(void)
{
    ansi_format_fg_color_rgb(color_prompt, sizeof(color_prompt),
                             COLOR_PROMPT_R, COLOR_PROMPT_G, COLOR_PROMPT_B);
    ansi_format_fg_color_rgb(color_divider, sizeof(color_divider),
                             COLOR_DIVIDER_R, COLOR_DIVIDER_G, COLOR_DIVIDER_B);
    ansi_format_fg_color_rgb(color_error, sizeof(color_error),
                             COLOR_ERROR_R, COLOR_ERROR_G, COLOR_ERROR_B);
    ansi_format_fg_color_rgb(color_info, sizeof(color_info),
                             COLOR_INFO_R, COLOR_INFO_G, COLOR_INFO_B);
    ansi_format_fg_color_rgb(color_nil, sizeof(color_nil),
                             COLOR_NIL_R, COLOR_NIL_G, COLOR_NIL_B);
    ansi_format_fg_color_rgb(color_number, sizeof(color_number),
                             COLOR_NUMBER_R, COLOR_NUMBER_G, COLOR_NUMBER_B);
    ansi_format_fg_color_rgb(color_string, sizeof(color_string),
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
        return color_string;
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

/* Multi-line expression buffer */
static char expr_buffer[8192] = { 0 };
static int expr_pos = 0;

/* --- Completion --- */

static LispCompleteContext detect_context(const char *buffer, int cursor_pos)
{
    int i = cursor_pos - 1;

    /* Skip current word */
    while (i >= 0 && buffer[i] != ' ' && buffer[i] != '\t' && buffer[i] != '(' && buffer[i] != ')' &&
           buffer[i] != '\'' && buffer[i] != '`') {
        i--;
    }

    /* Skip whitespace */
    while (i >= 0 && (buffer[i] == ' ' || buffer[i] == '\t')) {
        i--;
    }

    if (i >= 0 && buffer[i] == '(') {
        return LISP_COMPLETE_CALLABLE;
    }

    /* Check for (set! ...) pattern */
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

/* --- Echo helper for viewport --- */

static void echo_to_viewport(const char *text)
{
    if (g_app && text) {
        repl_app_echo(g_app, text, strlen(text));
    }
}

#endif /* HAVE_BLOOM_BOBA */

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
    printf("Bloom Lisp Interpreter v%s\n", BLOOM_LISP_VERSION);
    printf("\n");
    printf("Usage:\n");
    printf("  bloom-repl                          Start interactive REPL\n");
    printf("  bloom-repl -e \"CODE\"                Execute CODE and exit\n");
    printf("  bloom-repl FILE [FILE...]           Execute FILE(s) and exit\n");
    printf("  bloom-repl FILE -- [ARG...]         Run FILE with script arguments\n");
    printf("  bloom-repl -h, --help               Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  bloom-repl                            # Start REPL\n");
    printf("  bloom-repl -e \"(+ 1 2 3)\"             # Execute code\n");
    printf("  bloom-repl script.lisp                # Run file\n");
    printf("  bloom-repl script.lisp -- -i foo.txt  # Run with args in *command-line-args*\n");
    printf("  bloom-repl -e \"(define x 10) (* x 5)\" # Multiple expressions\n");
    printf("\n");
    printf("REPL Commands:\n");
    printf("  :quit             Exit the REPL\n");
    printf("  :load <filename>  Load and execute a Lisp file\n");
    printf("\n");
    printf("See LANGUAGE_REFERENCE.md for complete language documentation.\n");
}

#ifdef HAVE_BLOOM_BOBA

static int handle_command(const char *input, Environment *env)
{
    while (*input == ' ' || *input == '\t')
        input++;

    if (strncmp(input, ":quit", 5) == 0) {
        return 1;
    }

    if (strncmp(input, ":load", 5) == 0) {
        const char *filename = input + 5;
        while (*filename == ' ' || *filename == '\t')
            filename++;

        if (*filename == '\0') {
            char buf[256];
            snprintf(buf, sizeof(buf), "%sERROR: :load requires a filename%s\n",
                     color_error, SGR_RESET);
            echo_to_viewport(buf);
            return 0;
        }

        char *fname = GC_strdup(filename);
        LispObject *result = lisp_load_file(fname, env);

        if (result->type == LISP_ERROR) {
            char *err_str = lisp_print(result);
            char buf[4096];
            snprintf(buf, sizeof(buf), "%sERROR: %s%s\n",
                     color_error, err_str, SGR_RESET);
            echo_to_viewport(buf);
        } else {
            char *output = lisp_print(result);
            const char *clr = color_for_type(result->type);
            char buf[4096];
            snprintf(buf, sizeof(buf), "%s%s%s\n", clr, output, SGR_RESET);
            echo_to_viewport(buf);
        }

        return 0;
    }

    return -1;
}

/* --- Runtime command handler --- */

static void handle_line_submit(char *line)
{
    /* Skip blank lines if not accumulating */
    if (expr_pos == 0) {
        int blank = !line || line[0] == '\0';
        if (!blank) {
            blank = 1;
            for (const char *c = line; *c; c++) {
                if (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\r') {
                    blank = 0;
                    break;
                }
            }
        }
        if (blank) {
            tui_runtime_flush(g_runtime);
            return;
        }
    }

    /* Echo the input line to viewport */
    if (line && strchr(line, '\n')) {
        const char *p = line;
        int first = 1;
        while (*p) {
            const char *nl = strchr(p, '\n');
            int len = nl ? (int)(nl - p) : (int)strlen(p);
            const char *prompt = (first && expr_pos == 0) ? ">>> " : "... ";
            char echo_buf[8320];
            snprintf(echo_buf, sizeof(echo_buf), "%s%s%s%.*s\n",
                     color_prompt, prompt, SGR_RESET, len, p);
            echo_to_viewport(echo_buf);
            first = 0;
            p = nl ? nl + 1 : p + len;
        }
    } else {
        const char *prompt = expr_pos > 0 ? "... " : ">>> ";
        char echo_buf[8320];
        snprintf(echo_buf, sizeof(echo_buf), "%s%s%s%s\n",
                 color_prompt, prompt, SGR_RESET, line ? line : "");
        echo_to_viewport(echo_buf);
    }

    /* Handle commands on first line */
    if (expr_pos == 0 && line) {
        int cmd_result = handle_command(line, g_env);
        if (cmd_result == 1) {
            tui_runtime_quit(g_runtime);
            return;
        } else if (cmd_result == 0) {
            tui_runtime_flush(g_runtime);
            return;
        }
    }

    /* Add to history (first line only, non-empty) */
    if (line && line[0] != '\0' && expr_pos == 0) {
        tui_textinput_history_add(g_app->textinput, line);
    }

    /* Accumulate input for multi-line expressions */
    size_t line_len = line ? strlen(line) : 0;
    if (line_len > 0 && expr_pos + (int)line_len < (int)sizeof(expr_buffer) - 2) {
        if (expr_pos > 0) {
            expr_buffer[expr_pos++] = ' ';
        }
        strcpy(expr_buffer + expr_pos, line);
        expr_pos += line_len;
    }

    /* Try to parse */
    const char *input_ptr = expr_buffer;
    LispObject *expr = lisp_read(&input_ptr);

    /* Unclosed expression → continue reading */
    if (expr != NULL && expr->type == LISP_ERROR && strstr(expr->value.error, "Unclosed") != NULL) {
        repl_app_set_prompt(g_app, "... ");
        tui_runtime_flush(g_runtime);
        return;
    }

    if (expr == NULL) {
        /* Check if buffer is only whitespace (nothing to continue) */
        int all_ws = 1;
        for (int i = 0; i < expr_pos; i++) {
            if (expr_buffer[i] != ' ' && expr_buffer[i] != '\t' &&
                expr_buffer[i] != '\n' && expr_buffer[i] != '\r') {
                all_ws = 0;
                break;
            }
        }
        if (expr_pos == 0 || all_ws) {
            expr_pos = 0;
            expr_buffer[0] = '\0';
            repl_app_set_prompt(g_app, ">>> ");
            tui_runtime_flush(g_runtime);
            return;
        }
        repl_app_set_prompt(g_app, "... ");
        tui_runtime_flush(g_runtime);
        return;
    }

    if (expr->type == LISP_ERROR) {
        char err_buf[4096];
        snprintf(err_buf, sizeof(err_buf), "%sERROR: %s%s\n",
                 color_error, expr->value.error, SGR_RESET);
        echo_to_viewport(err_buf);
        expr_pos = 0;
        expr_buffer[0] = '\0';
        repl_app_set_prompt(g_app, ">>> ");
        tui_runtime_flush(g_runtime);
        return;
    }

    /* Capture stdout during eval */
    int pipefd[2];
    pipe(pipefd);
    int saved_stdout = dup(STDOUT_FILENO);
    dup2(pipefd[1], STDOUT_FILENO);

    LispObject *eval_result = lisp_eval(expr, g_env);
    fflush(stdout);

    /* Restore stdout */
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
    close(pipefd[1]);

    /* Read captured output */
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    char captured[4096];
    ssize_t n;
    while ((n = read(pipefd[0], captured, sizeof(captured) - 1)) > 0) {
        captured[n] = '\0';
        echo_to_viewport(captured);
    }
    close(pipefd[0]);

    /* Display result */
    if (eval_result->type == LISP_ERROR && !LISP_ERROR_CAUGHT(eval_result)) {
        char *err_str = lisp_print(eval_result);
        char err_buf[4096];
        snprintf(err_buf, sizeof(err_buf), "%sERROR: %s%s\n",
                 color_error, err_str, SGR_RESET);
        echo_to_viewport(err_buf);
    } else {
        char *output = lisp_print(eval_result);
        const char *clr = color_for_type(eval_result->type);
        char out_buf[4096];
        snprintf(out_buf, sizeof(out_buf), "%s%s%s\n", clr, output, SGR_RESET);
        echo_to_viewport(out_buf);
    }

    /* Reset buffer */
    expr_pos = 0;
    expr_buffer[0] = '\0';
    repl_app_set_prompt(g_app, ">>> ");
    tui_runtime_flush(g_runtime);
}

static void handle_tab_complete(TuiCmd *cmd)
{
    char *prefix = cmd->payload.tab_complete.prefix;
    int word_start = cmd->payload.tab_complete.word_start;

    /* Get the full input buffer for context detection */
    const char *buffer = tui_textinput_text(g_app->textinput);
    int cursor_pos = (int)tui_textinput_cursor(g_app->textinput);

    LispCompleteContext ctx = detect_context(buffer, cursor_pos);
    char **completions = lisp_get_completions(g_env, prefix, ctx);

    if (completions && completions[0]) {
        int count = 0;
        while (completions[count])
            count++;

        if (count == 1) {
            /* Single match — insert directly */
            tui_textinput_insert_completion(g_app->textinput, word_start, completions[0]);
        } else {
            /* Compute longest common prefix */
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
                /* Common prefix extends input — insert it */
                char *common = malloc(common_len + 1);
                if (common) {
                    memcpy(common, first, common_len);
                    common[common_len] = '\0';
                    tui_textinput_insert_completion(g_app->textinput, word_start, common);
                    free(common);
                }
            } else {
                /* No extension — display tabular list */
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

                char list_buf[8192];
                int pos = 0;
                pos += snprintf(list_buf, sizeof(list_buf), "%s", color_function);
                for (int c = 0; c < count; c++) {
                    int last_in_row = ((c + 1) % cols == 0) || (c == count - 1);
                    int sn;
                    if (last_in_row) {
                        sn = snprintf(list_buf + pos, sizeof(list_buf) - pos,
                                      "%s%s\n%s", completions[c], SGR_RESET,
                                      (c < count - 1) ? color_function : "");
                    } else {
                        sn = snprintf(list_buf + pos, sizeof(list_buf) - pos, "%-*s", col_width, completions[c]);
                    }
                    if (sn > 0 && pos + sn < (int)sizeof(list_buf))
                        pos += sn;
                }
                list_buf[pos] = '\0';
                echo_to_viewport(list_buf);
            }
        }

        /* Free completions */
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
    /* g_app is owned by the runtime — just null our pointer */
    g_app = NULL;

    if (g_runtime) {
        tui_runtime_free(g_runtime);
        g_runtime = NULL;
    }
}

/* --- Interactive REPL --- */

static void run_interactive_repl(Environment *env)
{
    (void)env;

    init_colors();

    ReplAppConfig app_config = {
        .terminal_width = 80,
        .terminal_height = 24,
    };
    TuiRuntimeConfig runtime_config = {
        .use_alternate_screen = 1,
        .raw_mode = 1,
        .enable_mouse = 1,
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

    /* Set REPL colors */
    tui_textinput_set_divider_color(g_app->textinput, color_divider);
    tui_textinput_set_prompt_color(g_app->textinput, color_prompt);

    /* Register cleanup */
    atexit(cleanup);

    /* Welcome message */
    char welcome[512];
    snprintf(welcome, sizeof(welcome),
             "%sBloom Lisp Interpreter v%s\n"
             "Type expressions to evaluate, :quit to exit, :load <file> to load a file\n"
             "Tab for completion, Up/Down for history, PageUp/PageDown to scroll%s\n\n",
             color_info, BLOOM_LISP_VERSION, SGR_RESET);
    repl_app_echo(g_app, welcome, strlen(welcome));

    /* Blocking event loop — runtime owns raw mode, signals, select() */
    tui_runtime_run(g_runtime);
}

#endif /* HAVE_BLOOM_BOBA */

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
#ifdef HAVE_BLOOM_BOBA
    g_env = env;
#endif

    env_define(env, lisp_intern("*command-line-args*")->value.symbol, NIL, pkg_user);

    /* Handle -e/--eval/-c flag */
    if (argc > 2 && (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "--eval") == 0 || strcmp(argv[1], "-c") == 0)) {
        const char *code = argv[2];
        const char *input = code;

        while (*input) {
            const char *parse_start = input;
            LispObject *expr = lisp_read(&input);

            if (expr == NULL || input == parse_start)
                break;

            if (expr->type == LISP_ERROR) {
                char *err_str = lisp_print(expr);
                fprintf(stderr, "ERROR: %s\n", err_str);
                lisp_cleanup();
                return 1;
            }

            LispObject *result = lisp_eval(expr, env);

            if (result->type == LISP_ERROR && !LISP_ERROR_CAUGHT(result)) {
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
            env_set(env, lisp_intern("*command-line-args*")->value.symbol, args_list);
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

                if (expr->type == LISP_ERROR) {
                    char *err_str = lisp_print(expr);
                    fprintf(stderr, "ERROR in %s: %s\n", argv[i], err_str);
                    return 1;
                }

                LispObject *result = lisp_eval(expr, env);

                if (result->type == LISP_ERROR && !LISP_ERROR_CAUGHT(result)) {
                    char *err_str = lisp_print(result);
                    fprintf(stderr, "ERROR in %s: %s\n", argv[i], err_str);
                    return 1;
                }
            }
        }

        lisp_cleanup();
        return 0;
    }

#ifdef HAVE_BLOOM_BOBA
    /* Interactive REPL */
    run_interactive_repl(env);

    env_free(env);
    lisp_cleanup();

    return 0;
#else
    fprintf(stderr, "Interactive REPL requires bloom-boba (batch mode only)\n");
    fprintf(stderr, "Usage: bloom-repl -e \"CODE\" or bloom-repl FILE\n");
    lisp_cleanup();
    return 1;
#endif
}
