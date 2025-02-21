#include <stdio.h>
#include <string.h>
#include "../src/lexer/lexer.h"

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

static int test_count = 0;
static int tests_passed = 0;

void assert_token(struct token *token, enum token_type expected_type, 
                 const char *expected_value, const char *test_name)
{
    test_count++;
    int passed = 1;
    
    if (token == NULL)
    {
        printf("%sTest %s: FAILED - Token is NULL%s\n", RED, test_name, RESET);
        return;
    }
    
    if (token->type != expected_type)
    {
        printf("%sTest %s: FAILED - Expected type %d, got %d%s\n", 
               RED, test_name, expected_type, token->type, RESET);
        passed = 0;
    }
    
    if (expected_value != NULL && (token->value == NULL || 
        strcmp(token->value, expected_value) != 0))
    {
        printf("%sTest %s: FAILED - Expected value '%s', got '%s'%s\n",
               RED, test_name, expected_value, 
               token->value ? token->value : "NULL", RESET);
        passed = 0;
    }
    
    if (passed)
    {
        printf("%sTest %s: PASSED%s\n", GREEN, test_name, RESET);
        tests_passed++;
    }
}

void test_simple_word(void)
{
    struct lexer *lexer = lexer_init("echo");
    struct token *token = lexer_next_token(lexer);
    assert_token(token, TOKEN_WORD, "echo", "Simple word");
    token_free(token);
    lexer_free(lexer);
}

void test_assignment(void)
{
    struct lexer *lexer = lexer_init("VAR=value");
    struct token *token = lexer_next_token(lexer);
    assert_token(token, TOKEN_ASSIGNMENT_WORD, "VAR=value", "Assignment word");
    token_free(token);
    lexer_free(lexer);
}

void test_operators(void)
{
    // Test single char operators
    char *operators[] = {"|", ">", "<", ";"};
    for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++)
    {
        struct lexer *lexer = lexer_init(operators[i]);
        struct token *token = lexer_next_token(lexer);
        char test_name[50];
        snprintf(test_name, sizeof(test_name), "Single operator '%s'", operators[i]);
        assert_token(token, TOKEN_OPERATOR, operators[i], test_name);
        token_free(token);
        lexer_free(lexer);
    }
    
    // Test double char operators
    char *double_operators[] = {">>", "&&", "||"};
    for (size_t i = 0; i < sizeof(double_operators) / sizeof(double_operators[0]); i++)
    {
        struct lexer *lexer = lexer_init(double_operators[i]);
        struct token *token = lexer_next_token(lexer);
        char test_name[50];
        snprintf(test_name, sizeof(test_name), "Double operator '%s'", double_operators[i]);
        assert_token(token, TOKEN_OPERATOR, double_operators[i], test_name);
        token_free(token);
        lexer_free(lexer);
    }
}

void test_ionumber(void)
{
    struct lexer *lexer = lexer_init("2>");
    struct token *token = lexer_next_token(lexer);
    assert_token(token, TOKEN_IONUMBER, "2", "IO Number");
    token_free(token);
    
    token = lexer_next_token(lexer);
    assert_token(token, TOKEN_OPERATOR, ">", "IO Redirection operator");
    token_free(token);
    lexer_free(lexer);
}

void test_complex_command(void)
{
    struct lexer *lexer = lexer_init("echo hello > output.txt");
    struct token *token;
    
    token = lexer_next_token(lexer);
    assert_token(token, TOKEN_WORD, "echo", "Complex - echo");
    token_free(token);
    
    token = lexer_next_token(lexer);
    assert_token(token, TOKEN_WORD, "hello", "Complex - hello");
    token_free(token);
    
    token = lexer_next_token(lexer);
    assert_token(token, TOKEN_OPERATOR, ">", "Complex - >");
    token_free(token);
    
    token = lexer_next_token(lexer);
    assert_token(token, TOKEN_WORD, "output.txt", "Complex - output.txt");
    token_free(token);
    
    token = lexer_next_token(lexer);
    assert_token(token, TOKEN_EOF, NULL, "Complex - EOF");
    token_free(token);
    
    lexer_free(lexer);
}

int main(void)
{
    printf("Running lexer tests...\n\n");
    
    test_simple_word();
    test_assignment();
    test_operators();
    test_ionumber();
    test_complex_command();
    
    printf("\nTests summary: %d/%d passed\n", tests_passed, test_count);
    return tests_passed == test_count ? 0 : 1;
}
