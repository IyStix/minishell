#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include "../src/exec/exec.h"

extern char **environ;

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

static int test_count = 0;
static int tests_passed = 0;

static char *capture_output(struct ast_node *node, struct exec_state *state)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
        return NULL;

    int stdout_save = dup(STDOUT_FILENO);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    exec_ast(node, state);
    fflush(stdout);

    dup2(stdout_save, STDOUT_FILENO);
    close(stdout_save);

    char *output = malloc(1024);
    ssize_t n = read(pipefd[0], output, 1023);
    if (n >= 0)
        output[n] = '\0';
    close(pipefd[0]);

    return output;
}

static void run_test(const char *command, const char *expected_output, const char *test_name)
{
    test_count++;

    struct lexer *lexer = lexer_init(strdup(command));
    struct parser *parser = parser_init(lexer);
    struct ast_node *ast = parse_input(parser);
    extern char **environ;
    struct exec_state *state = exec_init(environ);

    char *output = capture_output(ast, state);

    if (output && strcmp(output, expected_output) == 0)
    {
        printf("%sTest %s: PASSED%s\n", GREEN, test_name, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest %s: FAILED%s\n", RED, test_name, RESET);
        printf("Expected: '%s'\n", expected_output);
        printf("Got     : '%s'\n", output ? output : "NULL");
    }

    free(output);
    ast_node_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    exec_free(state);
}

static void test_echo(void)
{
    run_test("echo hello world", "hello world\n", "Echo simple");
    run_test("echo -n test", "test", "Echo -n option");
}

static void test_pipe(void)
{
    run_test("echo hello | cat", "hello\n", "Simple pipe");
    run_test("echo hello | cat | cat", "hello\n", "Multiple pipes");
}

static void test_redirections(void)
{
    system("rm -f test_out.txt");
    run_test("echo hello > test_out.txt; cat test_out.txt", "hello\n", "Output redirection");

    system("echo 'test input' > test_in.txt");
    run_test("cat < test_in.txt", "test input\n", "Input redirection");

    run_test("echo first > test_append.txt; echo second >> test_append.txt; cat test_append.txt",
            "first\nsecond\n", "Append redirection");

    system("rm -f test_out.txt test_in.txt test_append.txt");
}

static void test_and_or(void)
{
    run_test("true && echo success", "success\n", "AND operator success");
    run_test("false && echo should_not_print", "", "AND operator failure");
    run_test("false || echo success", "success\n", "OR operator success");
    run_test("true || echo should_not_print", "", "OR operator short-circuit");
}

static void test_sequences(void)
{
    run_test("echo first; echo second", "first\nsecond\n", "Simple sequence");
    run_test("echo a; echo b; echo c", "a\nb\nc\n", "Multiple sequence");
}

static void test_builtins(void)
{
    run_test("echo test builtin", "test builtin\n", "Echo builtin");

    char old_pwd[1024], new_pwd[1024];
    getcwd(old_pwd, sizeof(old_pwd));
    
    struct lexer *cd_lexer = lexer_init("cd ..");
    struct parser *cd_parser = parser_init(cd_lexer);
    struct ast_node *cd_ast = parse_input(cd_parser);
    struct exec_state *cd_state = exec_init(environ);
    
    exec_ast(cd_ast, cd_state);
    
    getcwd(new_pwd, sizeof(new_pwd));
    
    test_count++;
    if (strcmp(old_pwd, new_pwd) != 0)
    {
        printf("%sTest CD directory change: PASSED%s\n", GREEN, RESET);
        tests_passed++;
    }
    else
    {
        printf("%sTest CD directory change: FAILED%s\n", RED, RESET);
    }
    
    ast_node_free(cd_ast);
    parser_free(cd_parser);
    lexer_free(cd_lexer);
    exec_free(cd_state);
    
    chdir(old_pwd);
}

int main(void)
{
    printf("Running exec tests...\n\n");

    test_echo();
    test_pipe();
    test_redirections();
    test_and_or();
    test_sequences();
    test_builtins();

    printf("\nTests summary: %d/%d passed\n", tests_passed, test_count);
    return tests_passed == test_count ? 0 : 1;
}
