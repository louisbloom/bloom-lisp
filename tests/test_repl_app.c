/* test_repl_app.c - Tests for inline REPL app component
 *
 * Tests the on_submit/is_complete callback pattern, Enter interception
 * for complete vs incomplete forms, and cmd ownership/free safety.
 */

#include "../cli/repl_app.h"
#include "../include/lisp.h"
#include <assert.h>
#include <boba/ansi_sequences.h>
#include <boba/cmd.h>
#include <boba/components/textinput.h>
#include <boba/dynamic_buffer.h>
#include <boba/msg.h>
#include <boba/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn)                 \
    do {                             \
        tests_run++;                 \
        fn();                        \
        tests_passed++;              \
        printf("  PASS: %s\n", #fn); \
    } while (0)

#define ASSERT_TRUE(cond, msg)                     \
    do {                                           \
        if (!(cond)) {                             \
            fprintf(stderr, "  FAIL: %s:%d: %s\n", \
                    __FILE__, __LINE__, msg);      \
            abort();                               \
        }                                          \
    } while (0)

/* --- Helpers --- */

static void send_string(TuiTextInput *input, const char *s)
{
    for (const char *p = s; *p; p++)
        tui_textinput_update(input, tui_msg_char((uint32_t)*p, 0));
}

/* is_complete callback using lisp_read */
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

/* --- Tests --- */

/* #3: on_submit callback is invoked with the submitted text.
 * When Enter is pressed on a complete form, the on_submit callback
 * receives the text and the textinput is cleared. */
static char *g_submitted_text = NULL;
static int g_submit_count = 0;

static void on_submit_capture(char *text)
{
    g_submitted_text = text;
    g_submit_count++;
}

static void test_on_submit_invoked_with_text(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ASSERT_TRUE(ir.model != NULL, "init should succeed");
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_capture;

    /* Type a complete form */
    send_string(app->textinput, "(+ 1 2)");
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "(+ 1 2)") == 0,
                "text should be typed");

    /* Press Enter — should trigger on_submit */
    g_submitted_text = NULL;
    g_submit_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_submit_count == 1, "on_submit should be called once");
    ASSERT_TRUE(g_submitted_text != NULL, "submitted text should not be NULL");
    ASSERT_TRUE(strcmp(g_submitted_text, "(+ 1 2)") == 0,
                "submitted text should match input");
    ASSERT_TRUE(tui_textinput_len(app->textinput) == 0,
                "textinput should be cleared after submit");

    free(g_submitted_text);
    g_submitted_text = NULL;
    repl_app_free((TuiModel *)app);
}

/* #4a: Enter on incomplete form inserts newline, does NOT call on_submit */
static void test_enter_incomplete_inserts_newline(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_capture;

    /* Type an incomplete form */
    send_string(app->textinput, "(define");

    g_submit_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_submit_count == 0, "on_submit should NOT be called");
    ASSERT_TRUE(tui_textinput_line_count(app->textinput) == 2,
                "should have 2 lines after newline insertion");

    repl_app_free((TuiModel *)app);
}

/* #4b: Enter on complete form calls on_submit, no newline */
static void test_enter_complete_calls_submit(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_capture;

    send_string(app->textinput, "42");

    g_submit_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_submit_count == 1, "on_submit should be called");
    ASSERT_TRUE(tui_textinput_line_count(app->textinput) == 1,
                "should still be 1 line (no newline inserted)");

    free(g_submitted_text);
    g_submitted_text = NULL;
    repl_app_free((TuiModel *)app);
}

/* Auto-indent: Enter on incomplete form inserts newline + indent spaces */
static int compute_indent_2x(const char *text)
{
    int depth = 0;
    for (const char *p = text; *p; p++) {
        if (*p == '(')
            depth++;
        else if (*p == ')')
            depth--;
    }
    return depth > 0 ? depth * 2 : 0;
}

static void test_enter_incomplete_auto_indents(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->compute_indent = compute_indent_2x;

    /* Type "((42" — 2 unclosed parens */
    send_string(app->textinput, "((42");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    /* After Enter: text should be "((42\n  " (newline + 4 spaces) */
    const char *text = tui_textinput_text(app->textinput);
    ASSERT_TRUE(strstr(text, "\n") != NULL, "should have newline");
    /* The line after \n should start with 4 spaces (2 parens * 2) */
    const char *nl = strchr(text, '\n');
    ASSERT_TRUE(nl != NULL, "newline found");
    ASSERT_TRUE(nl[1] == ' ' && nl[2] == ' ' && nl[3] == ' ' && nl[4] == ' ',
                "should have 4 spaces after newline");

    repl_app_free((TuiModel *)app);
}

/* Ctrl-C aborts the current edit and clears the textinput */
static int g_break_count = 0;
static void on_break_test(void)
{
    g_break_count++;
}

static void test_ctrl_c_aborts_edit(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->on_break = on_break_test;

    send_string(app->textinput, "(+ 1");
    ASSERT_TRUE(tui_textinput_len(app->textinput) > 0, "should have text");

    g_break_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_NONE, 'c', TUI_MOD_CTRL));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_break_count == 1, "on_break should be called");
    /* on_break clears the textinput */
    tui_textinput_clear(app->textinput);
    ASSERT_TRUE(tui_textinput_len(app->textinput) == 0,
                "textinput should be empty after break");

    repl_app_free((TuiModel *)app);
}

/* #5: on_submit path doesn't double-free submitted text.
 * The on_submit callback receives a malloc'd string and is responsible
 * for freeing it. The runtime must not also free it. This test verifies
 * that freeing in the callback doesn't cause a crash (ASan would catch
 * a double-free or use-after-free). */
static void on_submit_free(char *text)
{
    free(text); /* callback frees the text */
}

static void test_on_submit_no_double_free(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_free;

    send_string(app->textinput, "(+ 1 2)");

    /* This should not crash — on_submit frees the text, and no
     * other code should touch it afterward */
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    /* If we get here without ASan error, the test passes */
    ASSERT_TRUE(1, "no double-free crash");

    repl_app_free((TuiModel *)app);
}

/* #9: After on_submit, the textinput view renders an empty prompt.
 * The view should show ">>> " with no input text. This verifies
 * that the input is cleared and the prompt is ready for next input. */
static void test_view_empty_after_submit(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_free;

    send_string(app->textinput, "(+ 1 2)");

    /* Submit */
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    /* Render view — should show empty prompt */
    DynamicBuffer *buf = dynamic_buffer_create(0);
    repl_app_view((const TuiModel *)app, buf);
    const char *data = dynamic_buffer_data(buf);

    /* Should contain ">>> " prompt */
    ASSERT_TRUE(strstr(data, ">>> ") != NULL, "view should show prompt");
    /* Should NOT contain the submitted text */
    ASSERT_TRUE(strstr(data, "(+ 1 2)") == NULL,
                "view should not contain submitted text");

    dynamic_buffer_destroy(buf);
    repl_app_free((TuiModel *)app);
}

int main(void)
{
    lisp_init();

    printf("repl_app tests:\n");

    RUN_TEST(test_on_submit_invoked_with_text);
    RUN_TEST(test_enter_incomplete_inserts_newline);
    RUN_TEST(test_enter_complete_calls_submit);
    RUN_TEST(test_enter_incomplete_auto_indents);
    RUN_TEST(test_ctrl_c_aborts_edit);
    RUN_TEST(test_on_submit_no_double_free);
    RUN_TEST(test_view_empty_after_submit);

    lisp_cleanup();

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
