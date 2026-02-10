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

/* ReplApp configuration */
typedef struct
{
    int terminal_width;
    int terminal_height;
    TuiCompletionCallback completer;
    void *completer_data;
} ReplAppConfig;

/* ReplApp model - composes viewport + textinput */
typedef struct
{
    TuiModel base;

    TuiViewport *viewport;
    TuiTextInput *textinput;

    int terminal_width;
    int terminal_height;
} ReplAppModel;

/* Create a new ReplApp component */
ReplAppModel *repl_app_create(const ReplAppConfig *config);

/* Free ReplApp component */
void repl_app_free(ReplAppModel *app);

/* Update ReplApp with a message */
TuiUpdateResult repl_app_update(ReplAppModel *app, TuiMsg msg);

/* Render ReplApp to output buffer */
void repl_app_view(const ReplAppModel *app, DynamicBuffer *out);

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

#endif /* REPL_APP_H */
