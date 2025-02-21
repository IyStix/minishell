#ifndef EXEC_H
#define EXEC_H

#include "../all.h"
#include "../parser/parser.h"

struct exec_state {
    char **env;
    int last_return;
    int should_exit;
    int exit_code;
};

/* Fonctions principales de l'exécuteur */
struct exec_state *exec_init(char **env);
void exec_free(struct exec_state *state);
int exec_ast(struct ast_node *node, struct exec_state *state);

/* Fonctions d'exécution spécifiques */
int exec_command(struct command *cmd, struct exec_state *state);
int exec_pipeline(struct ast_node *node, struct exec_state *state);
int exec_and_or(struct ast_node *node, struct exec_state *state);

/* Utilitaires */
int handle_redirections(struct command *cmd);

#endif /* EXEC_H */
