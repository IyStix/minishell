#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

static int test_count = 0;
static int tests_passed = 0;

void assert_command(struct ast_node *node, const char *name, const char *test_name)
{
    test_count++;
    int passed = 1;

    if (!node)
    {
        printf("%sTest %s: FAILED - Node is NULL%s\n", RED, test_name, RESET);
        return;
    }

    if (node->type != NODE_COMMAND)
    {
        printf("%sTest %s: FAILED - Expected NODE_COMMAND, got %d%s\n",
               RED, test_name, node->type, RESET);
        passed = 0;
    }

    if (strcmp(node->data.command->name, name) != 0)
    {
        printf("%sTest %s: FAILED - Expected command name '%s', got '%s'%s\n",
               RED, test_name, name, node->data.command->name, RESET);
        passed = 0;
    }

    if (passed)
    {
        printf("%sTest %s: PASSED%s\n", GREEN, test_name, RESET);
        tests_passed++;
    }
}

void test_simple_command(void)
{
    struct lexer *lexer = lexer_init("ls -l");
    struct parser *parser = parser_init(lexer);
    struct ast_node *node = parse_command(parser);

    assert_command(node, "ls", "Simple command");

    // Test argument
    if (node && node->data.command->args_count == 2 &&
        strcmp(node->data.command->args[1], "-l") == 0)
    {
        test_count++;
        printf("%sTest Simple command arguments: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        test_count++;
        printf("%sTest Simple command arguments: FAILED%s\n", RED, RESET);
    }

    ast_node_free(node);
    parser_free(parser);
    lexer_free(lexer);
}

void test_command_with_redirection(void)
{
    struct lexer *lexer = lexer_init("echo hello > output.txt");
    struct parser *parser = parser_init(lexer);
    struct ast_node *node = parse_command(parser);

    assert_command(node, "echo", "Command with redirection");

    // Print debug info
    printf("Debug: redirections_count = %d\n", node->data.command->redirections_count);
    if (node && node->data.command->redirections_count > 0) {
        printf("Debug: operator = '%s', word = '%s'\n", 
            node->data.command->redirections[0]->operator,
            node->data.command->redirections[0]->word);
    }
    
    // Test redirection
    if (node && node->data.command->redirections_count == 1 &&
        strcmp(node->data.command->redirections[0]->operator, ">") == 0 &&
        strcmp(node->data.command->redirections[0]->word, "output.txt") == 0)
    {
        test_count++;
        printf("%sTest Command redirection: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        test_count++;
        printf("%sTest Command redirection: FAILED%s\n", RED, RESET);
    }

    ast_node_free(node);
    parser_free(parser);
    lexer_free(lexer);
}

void test_command_with_io_number(void)
{
    struct lexer *lexer = lexer_init("2> error.log");
    struct parser *parser = parser_init(lexer);
    struct redirection *redir = parse_redirection(parser);

    test_count++;
    if (redir && redir->ionumber == 2 &&
        strcmp(redir->operator, ">") == 0 &&
        strcmp(redir->word, "error.log") == 0)
    {
        printf("%sTest IO number redirection: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest IO number redirection: FAILED%s\n", RED, RESET);
    }

    if (redir)
    {
        free(redir->operator);
        free(redir->word);
        free(redir);
    }
    parser_free(parser);
    lexer_free(lexer);
}

void test_pipeline(void)
{
    struct lexer *lexer = lexer_init("ls -l | grep test");
    struct parser *parser = parser_init(lexer);
    struct ast_node *node = parse_pipeline(parser);

    test_count++;
    if (node && node->type == NODE_PIPELINE &&
        strcmp(node->data.binary.operator, "|") == 0 &&
        node->data.binary.left->type == NODE_COMMAND &&
        node->data.binary.right->type == NODE_COMMAND)
    {
        printf("%sTest Pipeline: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest Pipeline: FAILED%s\n", RED, RESET);
    }

    ast_node_free(node);
    parser_free(parser);
    lexer_free(lexer);
}

void test_and_or(void)
{
    struct lexer *lexer = lexer_init("ls -l && grep test");
    struct parser *parser = parser_init(lexer);
    struct ast_node *node = parse_and_or(parser);

    test_count++;
    if (node && node->type == NODE_AND_OR &&
        strcmp(node->data.binary.operator, "&&") == 0 &&
        node->data.binary.left->type == NODE_COMMAND &&
        node->data.binary.right->type == NODE_COMMAND)
    {
        printf("%sTest AND operator: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest AND operator: FAILED%s\n", RED, RESET);
    }

    ast_node_free(node);
    parser_free(parser);
    lexer_free(lexer);

    // Test OR
    lexer = lexer_init("ls -l || echo \"not found\"");
    parser = parser_init(lexer);
    node = parse_and_or(parser);

    test_count++;
    if (node && node->type == NODE_AND_OR &&
        strcmp(node->data.binary.operator, "||") == 0)
    {
        printf("%sTest OR operator: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest OR operator: FAILED%s\n", RED, RESET);
    }

    ast_node_free(node);
    parser_free(parser);
    lexer_free(lexer);
}

void test_command_sequence(void)
{
    struct lexer *lexer = lexer_init("echo hello ; ls -l");
    struct parser *parser = parser_init(lexer);
    struct ast_node *node = parse_list(parser);

    test_count++;
    if (node && node->type == NODE_SEQUENCE &&
        strcmp(node->data.binary.operator, ";") == 0 &&
        node->data.binary.left->type == NODE_COMMAND &&
        node->data.binary.right->type == NODE_COMMAND)
    {
        printf("%sTest Command sequence: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest Command sequence: FAILED%s\n", RED, RESET);
    }

    ast_node_free(node);
    parser_free(parser);
    lexer_free(lexer);
}

void test_complex_input(void)
{
    struct lexer *lexer = lexer_init("ls -l | grep test && echo success ; exit 0");
    struct parser *parser = parser_init(lexer);
    struct ast_node *node = parse_input(parser);

    test_count++;
    if (node && node->type == NODE_SEQUENCE &&
        node->data.binary.left->type == NODE_AND_OR)
    {
        printf("%sTest Complex input: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest Complex input: FAILED%s\n", RED, RESET);
    }

    ast_node_free(node);
    parser_free(parser);
    lexer_free(lexer);
}

int main(void)
{
    printf("Running parser tests...\n\n");

    test_simple_command();
    test_command_with_redirection();
    test_command_with_io_number();
    test_pipeline();
    test_and_or();
    test_command_sequence();
    test_complex_input();

    printf("\nTests summary: %d/%d passed\n", tests_passed, test_count);
    return tests_passed == test_count ? 0 : 1;
}
