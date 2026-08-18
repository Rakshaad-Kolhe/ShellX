#ifndef SHELLX_SHELL_H
#define SHELLX_SHELL_H

#define SHELLX_DEFAULT_PROMPT "ShellX$ "
#define SHELLX_HISTORY_FILE ".shellx_history"
#define SHELLX_RC_FILE ".shellxrc"

/*
 * Returns the current shell prompt.
 * If PS1 is set in the environment, returns getenv("PS1"),
 * otherwise returns SHELLX_DEFAULT_PROMPT.
 */
const char *get_prompt(void);

#endif
