#ifndef SHELLX_CONFIG_H
#define SHELLX_CONFIG_H

#define SHELLX_RC_FILE ".shellxrc"

/*
 * Initialize the shell configuration subsystem (including alias table).
 */
void initialize_shell_config(void);

/*
 * Determine the location of ~/.shellxrc and load it.
 * Missing file is handled silently and successfully.
 * Returns 0 on success (or missing file), non-zero on file read error.
 */
int load_shell_rc(void);

/*
 * Parse and apply a single line of configuration.
 * Handles comments (#), whitespace, environment assignments (NAME=VALUE, PS1=VALUE),
 * and alias definitions (alias NAME=VALUE).
 * Returns 0 on success/comment/blank, non-zero on malformed line.
 */
int apply_config_line(const char *line, const char *filename, int line_number);

/*
 * Destroy the shell configuration subsystem and free all allocated state.
 */
void destroy_shell_config(void);

#endif
