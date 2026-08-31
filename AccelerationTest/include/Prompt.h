/**
 * @file Prompt.h
 * @brief Terminal input that survives typos, EOF and pasted junk.
 */

#ifndef PROMPT_H
#define PROMPT_H

#include <stdbool.h>

/**
 * @brief Ask for a number in [min, max], repeating until one arrives.
 * @param quit_value returned on EOF (Ctrl-D) or on an empty line, so an
 *        operator can always back out of a menu
 */
int prompt_menu(const char *title, int min, int max, int quit_value);

/** Ask a yes/no question. Returns @p on_eof if stdin closes. */
bool prompt_yes_no(const char *question, bool on_eof);

/** Wait for Enter. */
void prompt_pause(const char *message);

#endif /* PROMPT_H */
