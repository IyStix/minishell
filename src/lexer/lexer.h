#ifndef LEXER_H
#define LEXER_H

enum token_type {
    TOKEN_WORD,
    TOKEN_ASSIGNMENT_WORD,
    TOKEN_IONUMBER,
    TOKEN_OPERATOR,
    TOKEN_EOF,
    TOKEN_ERROR
};

struct token {
    enum token_type type;
    char *value;
    size_t line;
    size_t column;
};

struct lexer {
    char *input;
    size_t position;
    size_t line;
    size_t column;
    int has_error;
};

struct lexer *lexer_init(char *input);
void lexer_free(struct lexer *lexer);
struct token *lexer_next_token(struct lexer *lexer);
void token_free(struct token *token);

int is_word_char(char c);
int is_operator_char(char c);
int is_whitespace(char c);

#endif /* LEXER_H */
