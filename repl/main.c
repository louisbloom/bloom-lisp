#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "file_utils.h"
#include "lisp.h"
#include "repl_app.h"
#include <bloom-boba/ansi_sequences.h>
#include <bloom-boba/cmd.h>
#include <bloom-boba/dynamic_buffer.h>
#include <bloom-boba/input_parser.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/* Version information - fallback if not defined by autoconf */
#ifndef BLOOM_LISP_VERSION
#define BLOOM_LISP_VERSION "unknown"
#endif

/* Global state */
static Environment *g_env = NULL;
static ReplAppModel *g_app = NULL;
static TuiInputParser *g_input_parser = NULL;
static DynamicBuffer *g_render_buf = NULL;
static volatile sig_atomic_t g_quit_requested = 0;
static volatile sig_atomic_t g_resize_requested = 0;
static int g_term_rows = 24;
static int g_term_cols = 80;
static struct termios g_orig_termios;
static int g_raw_mode = 0;

/* --- Terminal handling --- */

static void enable_raw_mode(void)
{
    if (g_raw_mode)
        return;
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    struct termios raw = g_orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    g_raw_mode = 1;
}

static void disable_raw_mode(void)
{
    if (g_raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_raw_mode = 0;
    }
}

static void update_terminal_size(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        g_term_cols = ws.ws_col;
        g_term_rows = ws.ws_row;
    }
}

/* --- Signal handlers --- */

static void handle_sigint(int sig)
{
    (void)sig;
    g_quit_requested = 1;
}

static void handle_sigwinch(int sig)
{
    (void)sig;
    g_resize_requested = 1;
}

/* --- Rendering --- */

static void render_full_screen(void)
{
    if (!g_app || !g_render_buf)
        return;

    dynamic_buffer_clear(g_render_buf);

    /* Hide cursor during render */
    dynamic_buffer_append_str(g_render_buf, CSI "?25l");

    repl_app_view(g_app, g_render_buf);

    /* Show cursor */
    dynamic_buffer_append_str(g_render_buf, CSI "?25h");

    fwrite(g_render_buf->data, 1, g_render_buf->len, stdout);
    fflush(stdout);
}

/* --- Cleanup --- */

static void cleanup(void)
{
    disable_raw_mode();

    /* Disable mouse mode */
    fprintf(stdout, CSI "?1006l" CSI "?1000l");

    /* Exit alternate screen */
    fprintf(stdout, CSI "?1049l");
    fflush(stdout);

    if (g_app) {
        repl_app_free(g_app);
        g_app = NULL;
    }
    if (g_input_parser) {
        tui_input_parser_free(g_input_parser);
        g_input_parser = NULL;
    }
    if (g_render_buf) {
        dynamic_buffer_destroy(g_render_buf);
        g_render_buf = NULL;
    }
}

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

static int find_word_start(const char *buffer, int cursor_pos)
{
    int i = cursor_pos;
    while (i > 0 && buffer[i - 1] != ' ' && buffer[i - 1] != '\t' && buffer[i - 1] != '(' && buffer[i - 1] != ')' &&
           buffer[i - 1] != '\'' && buffer[i - 1] != '`') {
        i--;
    }
    return i;
}

/* --- Echo helper for viewport --- */

static void echo_to_viewport(const char *text)
{
    if (g_app && text) {
        repl_app_echo(g_app, text, strlen(text));
    }
}

/* --- Non-interactive helpers (unchanged from original) --- */

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
            echo_to_viewport("ERROR: :load requires a filename\n");
            return 0;
        }

        char *fname = GC_strdup(filename);
        LispObject *result = lisp_load_file(fname, env);

        if (result->type == LISP_ERROR) {
            char *err_str = lisp_print(result);
            char buf[4096];
            snprintf(buf, sizeof(buf), "ERROR: %s\n", err_str);
            echo_to_viewport(buf);
        } else {
            char *output = lisp_print(result);
            char buf[4096];
            snprintf(buf, sizeof(buf), "%s\n", output);
            echo_to_viewport(buf);
        }

        return 0;
    }

    return -1;
}

/* --- Interactive REPL --- */

static void run_interactive_repl(Environment *env)
{
    /* Get terminal size */
    update_terminal_size();

    /* Create boba components */
    g_input_parser = tui_input_parser_create();
    g_render_buf = dynamic_buffer_create(4096);

    ReplAppConfig config = {
        .terminal_width = g_term_cols,
        .terminal_height = g_term_rows,
    };
    g_app = repl_app_create(&config);

    if (!g_app || !g_input_parser || !g_render_buf) {
        fprintf(stderr, "ERROR: Failed to create TUI components\n");
        return;
    }

    /* Register cleanup */
    atexit(cleanup);

    /* Signal handlers */
    signal(SIGINT, handle_sigint);
    signal(SIGWINCH, handle_sigwinch);

    /* Enter alternate screen, clear, home cursor */
    fprintf(stdout, CSI "?1049h" CSI "2J" CSI "H");

    /* Enable mouse (SGR extended mode) */
    fprintf(stdout, CSI "?1000h" CSI "?1006h");
    fflush(stdout);

    /* Enter raw mode */
    enable_raw_mode();

    /* Welcome message */
    char welcome[256];
    snprintf(welcome, sizeof(welcome),
             "Bloom Lisp Interpreter v%s\n"
             "Type expressions to evaluate, :quit to exit, :load <file> to load a file\n"
             "Tab for completion, Up/Down for history, PageUp/PageDown to scroll\n\n",
             BLOOM_LISP_VERSION);
    repl_app_echo(g_app, welcome, strlen(welcome));

    /* Initial render */
    render_full_screen();

    /* Multi-line expression buffer */
    static char expr_buffer[8192] = { 0 };
    static int expr_pos = 0;

    /* Event loop */
    while (!g_quit_requested) {
        /* Handle pending resize */
        if (g_resize_requested) {
            g_resize_requested = 0;
            update_terminal_size();
            repl_app_set_terminal_size(g_app, g_term_cols, g_term_rows);
            render_full_screen();
        }

        /* Wait for stdin with timeout (100ms for signal responsiveness) */
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval tv = { 0, 100000 };

        int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        if (ready <= 0)
            continue;

        /* Read available bytes from stdin */
        unsigned char buf[256];
        ssize_t nread = read(STDIN_FILENO, buf, sizeof(buf));
        if (nread <= 0)
            continue;

        /* Feed each byte to input parser */
        for (ssize_t i = 0; i < nread; i++) {
            TuiMsg msg;
            if (!tui_input_parser_feed(g_input_parser, buf[i], &msg))
                continue;

            /* Mouse wheel → scroll viewport */
            if (msg.type == TUI_MSG_MOUSE) {
                if (msg.data.mouse.button == TUI_MOUSE_WHEEL_UP) {
                    repl_app_scroll_up(g_app, 3);
                    render_full_screen();
                    continue;
                }
                if (msg.data.mouse.button == TUI_MOUSE_WHEEL_DOWN) {
                    repl_app_scroll_down(g_app, 3);
                    render_full_screen();
                    continue;
                }
                continue;
            }

            /* PageUp/PageDown → page viewport */
            if (msg.type == TUI_MSG_KEY_PRESS) {
                if (msg.data.key.key == TUI_KEY_PAGE_UP) {
                    repl_app_page_up(g_app);
                    render_full_screen();
                    continue;
                }
                if (msg.data.key.key == TUI_KEY_PAGE_DOWN) {
                    repl_app_page_down(g_app);
                    render_full_screen();
                    continue;
                }
            }

            /* Route to app update */
            TuiUpdateResult result = repl_app_update(g_app, msg);

            /* Handle commands from textinput */
            if (result.cmd) {
                if (result.cmd->type == TUI_CMD_LINE_SUBMIT) {
                    char *line = result.cmd->payload.line;

                    /* Skip empty lines if not accumulating */
                    if ((!line || line[0] == '\0') && expr_pos == 0) {
                        tui_cmd_free(result.cmd);
                        render_full_screen();
                        continue;
                    }

                    /* Echo the input line to viewport */
                    {
                        const char *prompt = expr_pos > 0 ? "... " : ">>> ";
                        char echo_buf[8320];
                        snprintf(echo_buf, sizeof(echo_buf), "%s%s\n", prompt, line ? line : "");
                        echo_to_viewport(echo_buf);
                    }

                    /* Handle commands on first line */
                    if (expr_pos == 0 && line) {
                        int cmd_result = handle_command(line, env);
                        if (cmd_result == 1) {
                            tui_cmd_free(result.cmd);
                            g_quit_requested = 1;
                            break;
                        } else if (cmd_result == 0) {
                            tui_cmd_free(result.cmd);
                            render_full_screen();
                            continue;
                        }
                    }

                    /* Add to history (first line only, non-empty) */
                    if (line && line[0] != '\0' && expr_pos == 0) {
                        tui_textinput_history_add(g_app->textinput, line);
                    }

                    /* Accumulate input for multi-line expressions */
                    size_t line_len = line ? strlen(line) : 0;
                    if (line_len > 0 && expr_pos + line_len < sizeof(expr_buffer) - 2) {
                        if (expr_pos > 0) {
                            expr_buffer[expr_pos++] = ' ';
                        }
                        strcpy(expr_buffer + expr_pos, line);
                        expr_pos += line_len;
                    }

                    tui_cmd_free(result.cmd);

                    /* Try to parse */
                    const char *input_ptr = expr_buffer;
                    LispObject *expr = lisp_read(&input_ptr);

                    /* Unclosed expression → continue reading */
                    if (expr != NULL && expr->type == LISP_ERROR && strstr(expr->value.error, "Unclosed") != NULL) {
                        repl_app_set_prompt(g_app, "... ");
                        render_full_screen();
                        continue;
                    }

                    if (expr == NULL) {
                        if (expr_pos == 0) {
                            render_full_screen();
                            continue;
                        }
                        repl_app_set_prompt(g_app, "... ");
                        render_full_screen();
                        continue;
                    }

                    if (expr->type == LISP_ERROR) {
                        char err_buf[4096];
                        snprintf(err_buf, sizeof(err_buf), "ERROR: %s\n", expr->value.error);
                        echo_to_viewport(err_buf);
                        expr_pos = 0;
                        expr_buffer[0] = '\0';
                        repl_app_set_prompt(g_app, ">>> ");
                        render_full_screen();
                        continue;
                    }

                    /* Capture stdout during eval */
                    int pipefd[2];
                    pipe(pipefd);
                    int saved_stdout = dup(STDOUT_FILENO);
                    dup2(pipefd[1], STDOUT_FILENO);

                    LispObject *eval_result = lisp_eval(expr, env);
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
                    if (eval_result->type == LISP_ERROR && !eval_result->value.error_with_stack.caught) {
                        char *err_str = lisp_print(eval_result);
                        char err_buf[4096];
                        snprintf(err_buf, sizeof(err_buf), "ERROR: %s\n", err_str);
                        echo_to_viewport(err_buf);
                    } else {
                        char *output = lisp_print(eval_result);
                        char out_buf[4096];
                        snprintf(out_buf, sizeof(out_buf), "%s\n", output);
                        echo_to_viewport(out_buf);
                    }

                    /* Reset buffer */
                    expr_pos = 0;
                    expr_buffer[0] = '\0';
                    repl_app_set_prompt(g_app, ">>> ");
                } else if (result.cmd->type == TUI_CMD_TAB_COMPLETE) {
                    char *prefix = result.cmd->payload.tab_complete.prefix;
                    int word_start = result.cmd->payload.tab_complete.word_start;

                    /* Get the full input buffer for context detection */
                    const char *buffer = tui_textinput_text(g_app->textinput);
                    int cursor_pos = (int)tui_textinput_cursor(g_app->textinput);

                    LispCompleteContext ctx = detect_context(buffer, cursor_pos);
                    char **completions = lisp_get_completions(env, prefix, ctx);

                    if (completions && completions[0]) {
                        int count = 0;
                        while (completions[count])
                            count++;

                        if (count == 1) {
                            /* Single match — insert directly */
                            tui_textinput_insert_completion(g_app->textinput,
                                                            word_start, completions[0]);
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
                                    tui_textinput_insert_completion(
                                        g_app->textinput, word_start, common);
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
                                int cols = g_term_cols / col_width;
                                if (cols < 1)
                                    cols = 1;

                                char list_buf[8192];
                                int pos = 0;
                                for (int c = 0; c < count; c++) {
                                    int last_in_row = ((c + 1) % cols == 0) || (c == count - 1);
                                    int n;
                                    if (last_in_row) {
                                        n = snprintf(list_buf + pos,
                                                     sizeof(list_buf) - pos,
                                                     "%s\n", completions[c]);
                                    } else {
                                        n = snprintf(list_buf + pos,
                                                     sizeof(list_buf) - pos,
                                                     "%-*s", col_width,
                                                     completions[c]);
                                    }
                                    if (n > 0 && pos + n < (int)sizeof(list_buf))
                                        pos += n;
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

                    tui_cmd_free(result.cmd);
                } else {
                    tui_cmd_free(result.cmd);
                }
            }

            render_full_screen();
        }
    }
}

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
    lisp_init();
    Environment *global = env_create_global();
    Environment *env = env_create_session(global);
    g_env = env;

    env_define(env, "*command-line-args*", NIL);

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

            if (result->type == LISP_ERROR && !result->value.error_with_stack.caught) {
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
            env_set(env, "*command-line-args*", args_list);
        }

        for (int i = 1; i < file_end; i++) {
            FILE *file = file_open(argv[i], "rb");
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

                if (result->type == LISP_ERROR && !result->value.error_with_stack.caught) {
                    char *err_str = lisp_print(result);
                    fprintf(stderr, "ERROR in %s: %s\n", argv[i], err_str);
                    return 1;
                }
            }
        }

        lisp_cleanup();
        return 0;
    }

    /* Interactive REPL */
    run_interactive_repl(env);

    env_free(env);
    lisp_cleanup();

    return 0;
}
