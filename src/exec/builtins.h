#ifndef BUILTINS_H
#define BUILTINS_H

#include "../all.h"
#include "exec.h"



int builtin_echo(char **args, int arg_count, struct exec_state *state);
int builtin_cd(char **args, int arg_count, struct exec_state *state);
int builtin_exit(char **args, int arg_count, struct exec_state *state);
int builtin_kill(char **args, int arg_count, struct exec_state *state);
int is_builtin(const char *cmd);

#endif /* BUILTINS_H */
