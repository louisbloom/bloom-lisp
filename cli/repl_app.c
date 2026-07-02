/* repl_app.c - Inline REPL TUI component implementation
 *
 * Textinput only, inline rendering (no alt screen, no viewport, no borders).
 * Output goes directly to stdout; terminal scrollback is the output history.
 */

#include "repl_app.h"
#include <boba/ansi_sequences.h>
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

    /* Create textinput (user input) — multiline so Enter can insert newlines */
    TuiTextInputConfig textinput_cfg = { .multiline = 1 };
    app->textinput = tui_textinput_create(&textinput_cfg);
    if (!app->textinput) {
        free(app);
        return tui_init_result_none(NULL);
    }
    tui_textinput_set_prompt(app->textinput, ">>> ");
    tui_textinput_set_continuation_prompt(app->textinput, "... ");
    tui_textinput_set_terminal_width(app->textinput, app->terminal_width);
    tui_textinput_set_soft_wrap(app->textinput, 1);
    tui_textinput_set_history_size(app->textinput, 500);
    tui_textinput_set_word_chars(app->textinput,
                                 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-*!?");

    return tui_init_result_none((TuiModel *)app);
}

void repl_app_free(TuiModel *model)
{
    ReplAppModel *app = (ReplAppModel *)model;
    if (!app)
        return;

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
        app->terminal_width = msg.data.size.width;
        app->terminal_height = msg.data.size.height;
        tui_textinput_set_terminal_width(app->textinput, app->terminal_width);
        return tui_update_result_none();
    }

    if (msg.type == TUI_MSG_KEY_PRESS) {
        /* Intercept Ctrl-C: abort current edit, start fresh prompt */
        if (msg.data.key.key == TUI_KEY_NONE &&
            msg.data.key.rune == 'c' &&
            (msg.data.key.mods & TUI_MOD_CTRL) && app->on_break) {
            app->on_break();
            return tui_update_result_none();
        }
        /* Intercept Enter: if the form is incomplete, insert a newline
         * instead of submitting. If complete and on_submit is set,
         * call on_submit instead of letting line_submit fire. */
        if (msg.data.key.key == TUI_KEY_ENTER &&
            !(msg.data.key.mods & TUI_MOD_SHIFT) && app->is_complete) {
            const char *text = tui_textinput_text(app->textinput);
            if (text && !app->is_complete(text)) {
                /* Incomplete — insert newline, then auto-indent */
                TuiUpdateResult r = tui_textinput_update(app->textinput,
                                                         tui_msg_key(TUI_KEY_ENTER, 0, TUI_MOD_SHIFT));
                if (r.cmd)
                    tui_cmd_free(r.cmd);
                /* Compute and insert auto-indent spaces */
                if (app->compute_indent) {
                    const char *after = tui_textinput_text(app->textinput);
                    int indent = app->compute_indent(after);
                    for (int i = 0; i < indent; i++)
                        tui_textinput_update(app->textinput, tui_msg_char(' ', 0));
                }
                return tui_update_result_none();
            }
            /* Complete — call on_submit with the text, then clear */
            if (app->on_submit) {
                char *saved = strdup(text ? text : "");
                tui_textinput_clear(app->textinput);
                app->on_submit(saved);
                return tui_update_result_none();
            }
        }
        return tui_textinput_update(app->textinput, msg);
    }

    return tui_update_result_none();
}

TuiView repl_app_view(const TuiModel *model, DynamicBuffer *out)
{
    const ReplAppModel *app = (const ReplAppModel *)model;
    if (!app || !out)
        return tui_view_default(out);

    /* Render just the textinput — no viewport, no borders */
    tui_textinput_view(app->textinput, out);

    /* Declare inline mode + bracketed paste. No alt screen, no mouse. */
    TuiView v = tui_view_default(out);
    v.render_mode = TUI_RENDER_INLINE;
    v.bracketed_paste = 1;
    v.cursor = tui_textinput_cursor_pos(app->textinput);
    return v;
}

void repl_app_set_prompt(ReplAppModel *app, const char *prompt)
{
    if (app && app->textinput)
        tui_textinput_set_prompt(app->textinput, prompt);
}

const TuiComponent *repl_app_component(void)
{
    static const TuiComponent comp = {
        .init = repl_app_init,
        .update = repl_app_update,
        .view = repl_app_view,
        .free = repl_app_free,
    };
    return &comp;
}
