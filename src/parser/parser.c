#include <stdlib.h>
#include <string.h>
#include "parser.h"

struct parser *parser_init(struct lexer *lexer)
{
    struct parser *parser = malloc(sizeof(struct parser));
    if (!parser)
        return NULL;
    
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    parser->has_error = 0;
    
    return parser;
}

static void parser_advance(struct parser *parser)
{
    token_free(parser->current_token);
    parser->current_token = lexer_next_token(parser->lexer);
}

struct command *create_command(void)
{
    struct command *cmd = calloc(1, sizeof(struct command));
    if (!cmd)
        return NULL;
    
    cmd->args = malloc(sizeof(char *));
    cmd->redirections = malloc(sizeof(struct redirection *));
    cmd->assignments = malloc(sizeof(char *));
    
    return cmd;
}

static struct ast_node *create_node(enum node_type type)
{
    struct ast_node *node = malloc(sizeof(struct ast_node));
    if (!node)
        return NULL;
    
    node->type = type;
    return node;
}


struct ast_node *parse_command(struct parser *parser)
{
    struct command *cmd = create_command();
    if (!cmd)
        return NULL;
    
    struct ast_node *node = create_node(NODE_COMMAND);
    if (!node)
    {
        free(cmd);
        return NULL;
    }
    
    if (parser->current_token->type == TOKEN_WORD)
    {
        cmd->name = strdup(parser->current_token->value);
        cmd->args[cmd->args_count++] = strdup(parser->current_token->value);
        parser_advance(parser);
        
        while (parser->current_token->type == TOKEN_WORD)
        {
            cmd->args = realloc(cmd->args, sizeof(char *) * (cmd->args_count + 1));
            cmd->args[cmd->args_count++] = strdup(parser->current_token->value);
            parser_advance(parser);
        }
    }
    
    while (parser->current_token->type == TOKEN_ASSIGNMENT_WORD ||
           parser->current_token->type == TOKEN_IONUMBER ||
           (parser->current_token->type == TOKEN_OPERATOR && 
            (strcmp(parser->current_token->value, "<") == 0 ||
             strcmp(parser->current_token->value, ">") == 0 ||
             strcmp(parser->current_token->value, ">>") == 0)))
    {
        if (parser->current_token->type == TOKEN_ASSIGNMENT_WORD)
        {
            cmd->assignments = realloc(cmd->assignments, 
                sizeof(char *) * (cmd->assignments_count + 1));
            cmd->assignments[cmd->assignments_count++] = 
                strdup(parser->current_token->value);
            parser_advance(parser);
        }
        else
        {
            struct redirection *redir = parse_redirection(parser);
            if (redir)
            {
                cmd->redirections = realloc(cmd->redirections,
                    sizeof(struct redirection *) * (cmd->redirections_count + 1));
                cmd->redirections[cmd->redirections_count++] = redir;
            }
        }
    }
    
    node->data.command = cmd;
    return node;
}


struct redirection *parse_redirection(struct parser *parser)
{
    struct redirection *redir = malloc(sizeof(struct redirection));
    if (!redir)
        return NULL;
    
    redir->ionumber = -1;
    if (parser->current_token->type == TOKEN_IONUMBER)
    {
        redir->ionumber = atoi(parser->current_token->value);
        parser_advance(parser);
    }
    
    if (parser->current_token->type == TOKEN_OPERATOR)
    {
        redir->operator = strdup(parser->current_token->value);
        parser_advance(parser);
    }
    else
    {
        free(redir);
        return NULL;
    }
    
    if (parser->current_token->type == TOKEN_WORD)
    {
        redir->word = strdup(parser->current_token->value);
        parser_advance(parser);
    }
    else
    {
        free(redir->operator);
        free(redir);
        return NULL;
    }
    
    return redir;
}

struct ast_node *parse_pipeline(struct parser *parser)
{
    struct ast_node *left = parse_command(parser);
    if (!left)
        return NULL;
    
    while (parser->current_token->type == TOKEN_OPERATOR && 
           strcmp(parser->current_token->value, "|") == 0)
    {
        parser_advance(parser);
        
        struct ast_node *right = parse_command(parser);
        if (!right)
        {
            ast_node_free(left);
            return NULL;
        }
        
        struct ast_node *pipe_node = create_node(NODE_PIPELINE);
        if (!pipe_node)
        {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        pipe_node->data.binary.left = left;
        pipe_node->data.binary.right = right;
        pipe_node->data.binary.operator = strdup("|");
        
        left = pipe_node;
    }
    
    return left;
}

struct ast_node *parse_and_or(struct parser *parser)
{
    struct ast_node *left = parse_pipeline(parser);
    if (!left)
        return NULL;
    
    while (parser->current_token->type == TOKEN_OPERATOR && 
           (strcmp(parser->current_token->value, "&&") == 0 ||
            strcmp(parser->current_token->value, "||") == 0))
    {
        char *op = strdup(parser->current_token->value);
        parser_advance(parser);
        
        struct ast_node *right = parse_pipeline(parser);
        if (!right)
        {
            free(op);
            ast_node_free(left);
            return NULL;
        }
        
        struct ast_node *and_or_node = create_node(NODE_AND_OR);
        if (!and_or_node)
        {
            free(op);
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        and_or_node->data.binary.left = left;
        and_or_node->data.binary.right = right;
        and_or_node->data.binary.operator = op;
        
        left = and_or_node;
    }
    
    return left;
}

struct ast_node *parse_list(struct parser *parser)
{
    struct ast_node *left = parse_and_or(parser);
    if (!left)
        return NULL;
    
    while (parser->current_token->type == TOKEN_OPERATOR && 
           strcmp(parser->current_token->value, ";") == 0)
    {
        parser_advance(parser);
        
        if (parser->current_token->type == TOKEN_EOF)
            break;
            
        struct ast_node *right = parse_and_or(parser);
        if (!right)
        {
            ast_node_free(left);
            return NULL;
        }
        
        struct ast_node *sequence_node = create_node(NODE_SEQUENCE);
        if (!sequence_node)
        {
            ast_node_free(left);
            ast_node_free(right);
            return NULL;
        }
        
        sequence_node->data.binary.left = left;
        sequence_node->data.binary.right = right;
        sequence_node->data.binary.operator = strdup(";");
        
        left = sequence_node;
    }
    
    return left;
}

struct ast_node *parse_input(struct parser *parser)
{
    if (parser->current_token->type == TOKEN_EOF)
        return NULL;
        
    struct ast_node *node = parse_list(parser);
    
    if (node && parser->current_token->type != TOKEN_EOF)
    {
        ast_node_free(node);
        parser->has_error = 1;
        return NULL;
    }
    
    return node;
}

void ast_node_free(struct ast_node *node)
{
    if (!node)
        return;
        
    switch (node->type)
    {
        case NODE_COMMAND:
            if (node->data.command)
            {
                free(node->data.command->name);
                for (int i = 0; i < node->data.command->args_count; i++)
                    free(node->data.command->args[i]);
                free(node->data.command->args);
                
                for (int i = 0; i < node->data.command->redirections_count; i++)
                {
                    free(node->data.command->redirections[i]->operator);
                    free(node->data.command->redirections[i]->word);
                    free(node->data.command->redirections[i]);
                }
                free(node->data.command->redirections);
                
                for (int i = 0; i < node->data.command->assignments_count; i++)
                    free(node->data.command->assignments[i]);
                free(node->data.command->assignments);
                
                free(node->data.command);
            }
            break;
            
        case NODE_PIPELINE:
        case NODE_AND_OR:
        case NODE_SEQUENCE:
            ast_node_free(node->data.binary.left);
            ast_node_free(node->data.binary.right);
            free(node->data.binary.operator);
            break;
            
        case NODE_REDIRECTION:
            if (node->data.redirection)
            {
                free(node->data.redirection->operator);
                free(node->data.redirection->word);
                free(node->data.redirection);
            }
            break;
            
        case NODE_ASSIGNMENT:
            free(node->data.assignment);
            break;
    }
    
    free(node);
}

void parser_free(struct parser *parser)
{
    if (parser)
    {
        if (parser->current_token)
            token_free(parser->current_token);
        free(parser);
    }
}
