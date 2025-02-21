#ifndef PARSER_H
#define PARSER_H

#include "../lexer/lexer.h"

enum node_type {
    NODE_COMMAND,
    NODE_PIPELINE,
    NODE_AND_OR,
    NODE_SEQUENCE,
    NODE_REDIRECTION,
    NODE_ASSIGNMENT
};

struct redirection {
    int ionumber;
    char *operator;
    char *word;
};

struct command {
    char *name;
    char **args;
    int args_count;
    struct redirection **redirections;
    int redirections_count;
    char **assignments;
    int assignments_count;
};

struct ast_node {
    enum node_type type;
    union {
        struct command *command;
        struct {
            struct ast_node *left;
            struct ast_node *right;
            char *operator;
        } binary;
        struct redirection *redirection;
        char *assignment;
    } data;
};

struct parser {
    struct lexer *lexer;
    struct token *current_token;
    int has_error;
};

struct parser *parser_init(struct lexer *lexer);
void parser_free(struct parser *parser);
void ast_node_free(struct ast_node *node);

struct ast_node *parse_command(struct parser *parser);
struct redirection *parse_redirection(struct parser *parser);
struct ast_node *parse_pipeline(struct parser *parser);
struct ast_node *parse_and_or(struct parser *parser);
struct ast_node *parse_list(struct parser *parser);
struct ast_node *parse_input(struct parser *parser);

#endif /* PARSER_H */
