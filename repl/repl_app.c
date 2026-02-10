/* repl_app.c - REPL TUI component implementation
 *
 * Viewport + textinput layout (no statusbar).
 * Mirrors telnet_app.c but simplified for REPL use.
 */

#include "repl_app.h"
#include <bloom-boba/ansi_sequences.h>
#include <stdlib.h>
#include <string.h>

#define REPL_APP_TYPE_ID (TUI_COMPONENT_TYPE_BASE + 20)

ReplAppModel *repl_app_create(const ReplAppConfig *config)
{
    ReplAppModel *app = (ReplAppModel *)malloc(sizeof(ReplAppModel));
    if (!app)
        return NULL;

    memset(app, 0, sizeof(ReplAppModel));
    app->base.type = REPL_APP_TYPE_ID;

    app->terminal_width = config && config->terminal_width > 0 ? config->terminal_width : 80;
    app->terminal_height = config && config->terminal_height > 0 ? config->terminal_height : 24;

    /* Create viewport (output display) */
    app->viewport = tui_viewport_create();
    if (!app->viewport) {
        free(app);
        return NULL;
    }

    /* Create textinput (user input) */
    app->textinput = tui_textinput_create(NULL);
    if (!app->textinput) {
        tui_viewport_free(app->viewport);
        free(app);
        return NULL;
    }
    tui_textinput_set_show_dividers(app->textinput, 1);
    tui_textinput_set_prompt(app->textinput, ">>> ");

    /* Configure layout */
    repl_app_set_terminal_size(app, app->terminal_width, app->terminal_height);

    tui_textinput_set_word_delimiters(app->textinput, " \t()'`");

    return app;
}

void repl_app_free(ReplAppModel *app)
{
    if (!app)
        return;

    if (app->viewport)
        tui_viewport_free(app->viewport);
    if (app->textinput)
        tui_textinput_free(app->textinput);
    free(app);
}

TuiUpdateResult repl_app_update(ReplAppModel *app, TuiMsg msg)
{
    if (!app)
        return tui_update_result_none();

    if (msg.type == TUI_MSG_WINDOW_SIZE) {
        repl_app_set_terminal_size(app, msg.data.size.width, msg.data.size.height);
        return tui_update_result_none();
    }

    if (msg.type == TUI_MSG_KEY_PRESS) {
        tui_viewport_scroll_to_bottom(app->viewport);
        return tui_textinput_update(app->textinput, msg);
    }

    return tui_update_result_none();
}

void repl_app_view(const ReplAppModel *app, DynamicBuffer *out)
{
    if (!app || !out)
        return;

    /* Render viewport (fills top of screen) */
    tui_viewport_view(app->viewport, out);

    /* Render textinput last so cursor is left at prompt */
    tui_textinput_view(app->textinput, out);
}

void repl_app_echo(ReplAppModel *app, const char *text, size_t len)
{
    if (!app || !text || len == 0)
        return;

    tui_viewport_append(app->viewport, text, len);
}

void repl_app_set_terminal_size(ReplAppModel *app, int width, int height)
{
    if (!app)
        return;

    app->terminal_width = width;
    app->terminal_height = height;

    /* Textinput with dividers = 3 rows */
    int textinput_h = tui_textinput_get_height(app->textinput);

    /* Viewport fills remaining space */
    int viewport_h = height - textinput_h;
    if (viewport_h < 1)
        viewport_h = 1;

    /* Position textinput at bottom, viewport at top */
    int textinput_row = height - 1; /* Input line (middle of 3 divider rows) */

    if (app->viewport) {
        tui_viewport_set_size(app->viewport, width, viewport_h);
        tui_viewport_set_render_position(app->viewport, 1, 1);
    }

    if (app->textinput) {
        tui_textinput_set_terminal_width(app->textinput, width);
        tui_textinput_set_terminal_row(app->textinput, textinput_row);
    }
}

void repl_app_set_prompt(ReplAppModel *app, const char *prompt)
{
    if (app && app->textinput)
        tui_textinput_set_prompt(app->textinput, prompt);
}

void repl_app_scroll_up(ReplAppModel *app, int lines)
{
    if (app && app->viewport)
        tui_viewport_scroll_up(app->viewport, lines);
}

void repl_app_scroll_down(ReplAppModel *app, int lines)
{
    if (app && app->viewport)
        tui_viewport_scroll_down(app->viewport, lines);
}

void repl_app_page_up(ReplAppModel *app)
{
    if (app && app->viewport)
        tui_viewport_page_up(app->viewport);
}

void repl_app_page_down(ReplAppModel *app)
{
    if (app && app->viewport)
        tui_viewport_page_down(app->viewport);
}
