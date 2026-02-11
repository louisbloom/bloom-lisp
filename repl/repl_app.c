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

TuiInitResult repl_app_init(void *config)
{
    const ReplAppConfig *cfg = (const ReplAppConfig *)config;

    ReplAppModel *app = (ReplAppModel *)malloc(sizeof(ReplAppModel));
    if (!app)
        return tui_init_result_none(NULL);

    memset(app, 0, sizeof(ReplAppModel));
    app->base.type = REPL_APP_TYPE_ID;

    app->terminal_width = cfg && cfg->terminal_width > 0 ? cfg->terminal_width : 80;
    app->terminal_height = cfg && cfg->terminal_height > 0 ? cfg->terminal_height : 24;

    /* Create viewport (output display) */
    app->viewport = tui_viewport_create();
    if (!app->viewport) {
        free(app);
        return tui_init_result_none(NULL);
    }

    /* Create textinput (user input) */
    app->textinput = tui_textinput_create(NULL);
    if (!app->textinput) {
        tui_viewport_free(app->viewport);
        free(app);
        return tui_init_result_none(NULL);
    }
    tui_textinput_set_show_dividers(app->textinput, 1);
    tui_textinput_set_prompt(app->textinput, ">>> ");

    /* Configure layout */
    repl_app_set_terminal_size(app, app->terminal_width, app->terminal_height);

    tui_textinput_set_word_delimiters(app->textinput, " \t()'`");

    return tui_init_result_none((TuiModel *)app);
}

void repl_app_free(TuiModel *model)
{
    ReplAppModel *app = (ReplAppModel *)model;
    if (!app)
        return;

    if (app->viewport)
        tui_viewport_free(app->viewport);
    if (app->textinput)
        tui_textinput_free(app->textinput);
    free(app);
}

TuiUpdateResult repl_app_update(TuiModel *model, TuiMsg msg)
{
    ReplAppModel *app = (ReplAppModel *)model;
    if (!app)
        return tui_update_result_none();

    if (msg.type == TUI_MSG_WINDOW_SIZE) {
        repl_app_set_terminal_size(app, msg.data.size.width, msg.data.size.height);
        return tui_update_result_none();
    }

    /* Mouse wheel → scroll viewport */
    if (msg.type == TUI_MSG_MOUSE) {
        if (msg.data.mouse.button == TUI_MOUSE_WHEEL_UP) {
            repl_app_scroll_up(app, 3);
        } else if (msg.data.mouse.button == TUI_MOUSE_WHEEL_DOWN) {
            repl_app_scroll_down(app, 3);
        }
        return tui_update_result_none();
    }

    if (msg.type == TUI_MSG_KEY_PRESS) {
        /* PageUp/PageDown → page viewport */
        if (msg.data.key.key == TUI_KEY_PAGE_UP) {
            repl_app_page_up(app);
            return tui_update_result_none();
        }
        if (msg.data.key.key == TUI_KEY_PAGE_DOWN) {
            repl_app_page_down(app);
            return tui_update_result_none();
        }

        tui_viewport_scroll_to_bottom(app->viewport);
        return tui_textinput_update(app->textinput, msg);
    }

    return tui_update_result_none();
}

void repl_app_view(const TuiModel *model, DynamicBuffer *out)
{
    const ReplAppModel *app = (const ReplAppModel *)model;
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

static const TuiComponent repl_app_component_instance = {
    .init = repl_app_init,
    .update = repl_app_update,
    .view = repl_app_view,
    .free = repl_app_free,
};

const TuiComponent *repl_app_component(void)
{
    return &repl_app_component_instance;
}
