#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

static char lexer_peek(struct lexer *lexer)
{
    if (lexer->position >= strlen(lexer->input))
        return '\0';
    return lexer->input[lexer->position];
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

static struct token *create_token(enum token_type type, char *value)
{
    struct token *token = malloc(sizeof(struct token));
    if (!token)
        return NULL;
    
    token->type = type;
    token->value = value;
    return token;
}

static size_t copy_str(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    dest[i] = '\0';
    return i;
}

static char *create_str_copy(const char *str, size_t len)
{
    char *new_str = malloc(len + 1);
    if (!new_str)
        return NULL;
    copy_str(new_str, str, len);
    return new_str;
}

static char *read_word_value(struct lexer *lexer, size_t start_pos, size_t length)
{
    return create_str_copy(lexer->input + start_pos, length);
}

struct token *lexer_next_token(struct lexer *lexer)
{
    char c = lexer_peek(lexer);
    size_t start_pos = lexer->position;
    
    while (is_whitespace(c))
    {
        lexer_advance(lexer);
        c = lexer_peek(lexer);
    }
    
    if (c == '\0')
        return create_token(TOKEN_EOF, NULL);
    
    if (is_word_char(c))
    {
        while (is_word_char(lexer_peek(lexer)))
            lexer_advance(lexer);
            
        size_t length = lexer->position - start_pos;
        char *value = read_word_value(lexer, start_pos, length);
        return create_token(TOKEN_WORD, value);
    }
    
    if (is_operator_char(c))
    {
        char next = lexer_peek(lexer);
        lexer_advance(lexer);
        
        if ((c == '>' && next == '>') ||
            (c == '&' && next == '&') ||
            (c == '|' && next == '|'))
        {
            lexer_advance(lexer);
            char *value = malloc(3);
            if (!value)
                return NULL;
            value[0] = c;
            value[1] = next;
            value[2] = '\0';
            return create_token(TOKEN_OPERATOR, value);
        }
        
        char *value = malloc(2);
        if (!value)
            return NULL;
        value[0] = c;
        value[1] = '\0';
        return create_token(TOKEN_OPERATOR, value);
    }
    
    lexer_advance(lexer);
    return create_token(TOKEN_ERROR, NULL);
}

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
           c == '_' || c == '-' || c == '.';
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
