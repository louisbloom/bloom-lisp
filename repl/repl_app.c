/* repl_app.c - REPL TUI component implementation
 *
 * Viewport + textinput layout (no statusbar).
 * Mirrors telnet_app.c but simplified for REPL use.
 */

#include "repl_app.h"
#include <bloom-boba/ansi_sequences.h>
#include <stdio.h>
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
    TuiTextInputConfig textinput_cfg = { .multiline = 1 };
    app->textinput = tui_textinput_create(&textinput_cfg);
    if (!app->textinput) {
        tui_viewport_free(app->viewport);
        free(app);
        return tui_init_result_none(NULL);
    }
    tui_textinput_set_prompt(app->textinput, ">>> ");
    tui_textinput_set_continuation_prompt(app->textinput, "... ");

    /* Border style: faint, mirroring the previous SGR_DIM divider color. */
    app->border_style = tui_style_faint(tui_style_new(), 1);

    /* Configure layout */
    repl_app_set_terminal_size(app, app->terminal_width, app->terminal_height);

    tui_textinput_set_word_chars(app->textinput,
                                 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-*!?");
    tui_textinput_set_history_size(app->textinput, 500);

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

        TuiUpdateResult result = tui_textinput_update(app->textinput, msg);
        /* Recalculate layout in case textinput height changed (multiline) */
        repl_app_set_terminal_size(app, app->terminal_width, app->terminal_height);
        tui_viewport_scroll_to_bottom(app->viewport);
        return result;
    }

    return tui_update_result_none();
}

/* Position cursor at (row, 1), clear-to-EOL, and write a styled border edge. */
static void render_border_at(DynamicBuffer *out, int row, int width,
                             int top, const TuiStyle *style)
{
    if (row <= 0 || width <= 0)
        return;
    char *line = tui_border_render_horizontal(&TUI_BORDER_NORMAL, top, width,
                                              style, NULL,
                                              TUI_BORDER_TITLE_LEFT, 0, 0);
    if (!line)
        return;
    char pos[32];
    snprintf(pos, sizeof(pos), CSI "%d;1H", row);
    dynamic_buffer_append_str(out, pos);
    dynamic_buffer_append_str(out, EL_TO_END);
    dynamic_buffer_append_str(out, line);
    free(line);
}

TuiView repl_app_view(const TuiModel *model, DynamicBuffer *out)
{
    const ReplAppModel *app = (const ReplAppModel *)model;
    if (!app || !out)
        return tui_view_default(out);

    /* Viewport on top, top border, textinput, bottom border. */
    tui_viewport_view(app->viewport, out);
    render_border_at(out, app->top_border_row, app->terminal_width, 1,
                     &app->border_style);
    tui_textinput_view(app->textinput, out);
    render_border_at(out, app->bottom_border_row, app->terminal_width, 0,
                     &app->border_style);

    /* Declare terminal modes for this frame. The REPL always wants
     * alt-screen + cell-motion mouse; cursor follows the textinput. */
    TuiView v = tui_view_default(out);
    v.alt_screen = 1;
    v.mouse_mode = TUI_MOUSE_MODE_CELL_MOTION;
    if (app->textinput)
        v.cursor = tui_textinput_cursor_pos(app->textinput);
    return v;
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

    int content_lines = tui_textinput_get_height(app->textinput);

    /* Layout, bottom-up: bottom border on the last row, then textinput
     * content rows above it, then top border, then the viewport above. */
    app->bottom_border_row = height;
    int textinput_row = height - content_lines; /* first content row */
    app->top_border_row = textinput_row - 1;

    /* Viewport fills the rest. */
    int viewport_h = app->top_border_row - 1;
    if (viewport_h < 1)
        viewport_h = 1;

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
