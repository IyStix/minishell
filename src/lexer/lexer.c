#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

struct lexer *lexer_init(char *input)
{
    struct lexer *lexer = malloc(sizeof(struct lexer));
    if (!lexer)
        return NULL;
        
    lexer->input = input;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->has_error = 0;
    
    return lexer;
}

void lexer_free(struct lexer *lexer)
{
    if (lexer)
        free(lexer);
}

static char lexer_peek(struct lexer *lexer)
{
    if (lexer->position >= strlen(lexer->input))
        return '\0';
    return lexer->input[lexer->position];
}

static char lexer_peek_next(struct lexer *lexer)
{
    if (lexer->position + 1 >= strlen(lexer->input))
        return '\0';
    return lexer->input[lexer->position + 1];
}

static char lexer_advance(struct lexer *lexer)
{
    char c = lexer_peek(lexer);
    if (c == '\n')
    {
        lexer->line++;
        lexer->column = 1;
    }
    else
    {
        lexer->column++;
    }
    lexer->position++;
    return c;
}

static void skip_whitespace(struct lexer *lexer)
{
    while (is_whitespace(lexer_peek(lexer)))
        lexer_advance(lexer);
}

static struct token *create_token(enum token_type type, char *value, size_t line, size_t column)
{
    struct token *token = malloc(sizeof(struct token));
    if (!token)
        return NULL;
    
    token->type = type;
    token->value = value;
    token->line = line;
    token->column = column;
    
    return token;
}

static struct token *read_word(struct lexer *lexer)
{
    size_t start_pos = lexer->position;
    size_t start_col = lexer->column;
    int is_assignment = 0;
    
    while (is_word_char(lexer_peek(lexer)))
    {
        if (lexer_peek_next(lexer) == '=')
        {
            is_assignment = 1;
            lexer_advance(lexer);
            break;
        }
        lexer_advance(lexer);
    }
    
    if (is_assignment)
    {
        lexer_advance(lexer);
        while (is_word_char(lexer_peek(lexer)))
            lexer_advance(lexer);
            
        size_t length = lexer->position - start_pos;
        char *value = strndup(lexer->input + start_pos, length);
        return create_token(TOKEN_ASSIGNMENT_WORD, value, lexer->line, start_col);
    }
    
    size_t length = lexer->position - start_pos;
    char *value = strndup(lexer->input + start_pos, length);
    return create_token(TOKEN_WORD, value, lexer->line, start_col);
}

static struct token *read_operator(struct lexer *lexer)
{
    size_t start_pos = lexer->position;
    size_t start_col = lexer->column;
    char curr = lexer_peek(lexer);
    char next = lexer_peek_next(lexer);
    
    if ((curr == '>' && next == '>') ||
        (curr == '&' && next == '&') ||
        (curr == '|' && next == '|'))
    {
        lexer_advance(lexer);
        lexer_advance(lexer);
        char *value = strndup(lexer->input + start_pos, 2);
        return create_token(TOKEN_OPERATOR, value, lexer->line, start_col);
    }
    
    if (curr == '|' || curr == '>' || curr == '<' || curr == ';')
    {
        lexer_advance(lexer);
        char *value = strndup(lexer->input + start_pos, 1);
        return create_token(TOKEN_OPERATOR, value, lexer->line, start_col);
    }
    
    return NULL;
}

static struct token *read_ionumber(struct lexer *lexer)
{
    size_t start_pos = lexer->position;
    size_t start_col = lexer->column;
    char next;
    
    while (isdigit(lexer_peek(lexer)))
        lexer_advance(lexer);
        
    next = lexer_peek(lexer);
    if (next == '>' || next == '<')
    {
        size_t length = lexer->position - start_pos;
        char *value = strndup(lexer->input + start_pos, length);
        return create_token(TOKEN_IONUMBER, value, lexer->line, start_col);
    }
    
    lexer->position = start_pos;
    lexer->column = start_col;
    return read_word(lexer);
}

struct token *lexer_next_token(struct lexer *lexer)
{
    skip_whitespace(lexer);
    
    char c = lexer_peek(lexer);
    if (c == '\0')
        return create_token(TOKEN_EOF, NULL, lexer->line, lexer->column);
        
    if (isdigit(c))
        return read_ionumber(lexer);
        
    if (is_word_char(c))
        return read_word(lexer);
        
    if (is_operator_char(c))
        return read_operator(lexer);
    
    lexer->has_error = 1;
    return create_token(TOKEN_ERROR, NULL, lexer->line, lexer->column);
}

void token_free(struct token *token)
{
    if (token)
    {
        free(token->value);
        free(token);
    }
}

int is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || 
           c == '_' || c == '-' || c == '.' || 
           c == '/' || c == '@' || c == '+';
}

int is_operator_char(char c)
{
    return c == '|' || c == '>' || c == '<' || 
           c == '&' || c == ';';
}

int is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}
