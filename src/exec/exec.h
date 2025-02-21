#ifndef EXEC_H
#define EXEC_H


#include "../parser/parser.h"

struct exec_state {
    char **env;
    int last_return; 
    int should_exit;
    int exit_code;
};

struct exec_state *exec_init(char **env);
void exec_free(struct exec_state *state);
int exec_ast(struct ast_node *node, struct exec_state *state);

int exec_command(struct command *cmd, struct exec_state *state);
int exec_pipeline(struct ast_node *node, struct exec_state *state);
int exec_and_or(struct ast_node *node, struct exec_state *state);
int exec_sequence(struct ast_node *node, struct exec_state *state);

int builtin_echo(char **args, int arg_count, struct exec_state *state);
int builtin_cd(char **args, int arg_count, struct exec_state *state);
int builtin_exit(char **args, int arg_count, struct exec_state *state);
int builtin_kill(char **args, int arg_count, struct exec_state *state);

int is_builtin(const char *cmd);
int handle_redirections(struct command *cmd);
void restore_redirections(int saved_fds[3]);
int expand_variables(char **word, struct exec_state *state);

#endif /* EXEC_H */
