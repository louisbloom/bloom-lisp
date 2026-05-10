/* repl_app.h - REPL TUI component: composes viewport + textinput
 *
 * Follows the same Elm Architecture composition pattern as telnet_app:
 * - Embeds child components (viewport, textinput) in its model
 * - Routes messages to children in update()
 * - Composes child views in view()
 */

#ifndef REPL_APP_H
#define REPL_APP_H

#include <stddef.h>

#include <bloom-boba/component.h>
#include <bloom-boba/components/textinput.h>
#include <bloom-boba/components/viewport.h>
#include <bloom-boba/style.h>

/* ReplApp configuration */
typedef struct
{
    int terminal_width;
    int terminal_height;
} ReplAppConfig;

/* ReplApp model - composes viewport + textinput */
typedef struct
{
    TuiModel base;

    TuiViewport *viewport;
    TuiTextInput *textinput;

    /* Style applied to the top + bottom border lines flanking the
     * textinput. Set via tui_style_* builders; callers may swap it at
     * any point to recolor the borders. */
    TuiStyle border_style;

    int terminal_width;
    int terminal_height;

    /* Computed in repl_app_set_terminal_size, consumed in repl_app_view. */
    int top_border_row;
    int bottom_border_row;
} ReplAppModel;

/* TuiComponent interface — init/update/view/free.
 *
 * view() returns a TuiView that declares alt-screen + mouse mode +
 * cursor (delegated to textinput). The cursor is populated inside
 * view(); there is no separate cursor() slot in v2. */
TuiInitResult repl_app_init(void *config);
TuiUpdateResult repl_app_update(TuiModel *model, TuiMsg msg);
TuiView repl_app_view(const TuiModel *model, DynamicBuffer *out);
void repl_app_free(TuiModel *model);

/* Echo text to the viewport */
void repl_app_echo(ReplAppModel *app, const char *text, size_t len);

/* Set terminal size (call on window resize) */
void repl_app_set_terminal_size(ReplAppModel *app, int width, int height);

/* Set the prompt string */
void repl_app_set_prompt(ReplAppModel *app, const char *prompt);

/* Scrolling control */
void repl_app_scroll_up(ReplAppModel *app, int lines);
void repl_app_scroll_down(ReplAppModel *app, int lines);
void repl_app_page_up(ReplAppModel *app);
void repl_app_page_down(ReplAppModel *app);

/* Get component interface for ReplApp (used by tui_runtime_create) */
const TuiComponent *repl_app_component(void);

#endif /* REPL_APP_H */
