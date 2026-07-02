/* repl_app.h - Inline REPL TUI component: textinput only, no alt screen
 *
 * Inline mode renders in the primary terminal buffer — no viewport,
 * no borders. Output goes directly to stdout; the terminal's own
 * scrollback is the output history.
 */

#ifndef REPL_APP_H
#define REPL_APP_H

#include <stddef.h>

#include <boba/component.h>
#include <boba/components/textinput.h>

/* ReplApp configuration */
typedef struct
{
    int terminal_width;
    int terminal_height;
} ReplAppConfig;

/* ReplApp model — just a textinput in inline mode */
typedef struct
{
    TuiModel base;
    TuiTextInput *textinput;
    int terminal_width;
    int terminal_height;

    /* Callback: returns 1 if text is a complete form, 0 if incomplete.
     * Used to decide whether Enter evaluates or inserts a newline. */
    int (*is_complete)(const char *text);

    /* Callback: returns the number of spaces to indent the new line
     * after Enter is pressed on an incomplete form. Receives the text
     * after the newline was inserted. */
    int (*compute_indent)(const char *text);

    /* Callback: called when Ctrl-C is pressed to abort the current
     * edit and start a fresh prompt. */
    void (*on_break)(void);

    void (*on_submit)(char *text);
} ReplAppModel;

/* TuiComponent interface */
TuiInitResult repl_app_init(void *config);
TuiUpdateResult repl_app_update(TuiModel *model, TuiMsg msg);
TuiView repl_app_view(const TuiModel *model, DynamicBuffer *out);
void repl_app_free(TuiModel *model);

/* Set the prompt string */
void repl_app_set_prompt(ReplAppModel *app, const char *prompt);

/* Get component interface for ReplApp */
const TuiComponent *repl_app_component(void);

#endif /* REPL_APP_H */
