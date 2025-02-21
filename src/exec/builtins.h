#ifndef BUILTINS_H
#define BUILTINS_H

#include "exec.h"

/* Vérification si une commande est un builtin */
int is_builtin(const char *cmd);

/* Built-ins */
int builtin_echo(char **args, int arg_count, struct exec_state *state);
int builtin_cd(char **args, int arg_count, struct exec_state *state);
int builtin_exit(char **args, int arg_count, struct exec_state *state);
int builtin_kill(char **args, int arg_count, struct exec_state *state);

#endif /* BUILTINS_H */
