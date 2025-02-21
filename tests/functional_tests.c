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

// Utilitaire pour capturer la sortie et comparer avec la référence (bash --posix)
static void run_test_case(const char *cmd, const char *test_name) 
{
    test_count++;
    
    // Préparer les pipes pour la capture
    int pipe_minishell[2];
    int pipe_bash[2];
    
    if (pipe(pipe_minishell) == -1 || pipe(pipe_bash) == -1) {
        perror("pipe");
        return;
    }

    // Fork pour bash --posix (référence)
    pid_t pid_bash = fork();
    if (pid_bash == 0) {
        dup2(pipe_bash[1], STDOUT_FILENO);
        dup2(pipe_bash[1], STDERR_FILENO);
        close(pipe_bash[0]);
        close(pipe_bash[1]);
        
        char *bash_args[] = {"bash", "--posix", "-c", (char *)cmd, NULL};
        execvp("bash", bash_args);
        exit(1);
    }

    // Fork pour notre minishell
    pid_t pid_minishell = fork();
    if (pid_minishell == 0) {
        dup2(pipe_minishell[1], STDOUT_FILENO);
        dup2(pipe_minishell[1], STDERR_FILENO);
        close(pipe_minishell[0]);
        close(pipe_minishell[1]);

        struct lexer *lexer = lexer_init(strdup(cmd));
        struct parser *parser = parser_init(lexer);
        struct ast_node *ast = parse_input(parser);
        struct exec_state *state = exec_init(environ);

        exec_ast(ast, state);
        
        ast_node_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        exec_free(state);
        exit(0);
    }

    // Parent
    close(pipe_minishell[1]);
    close(pipe_bash[1]);

    // Lire et comparer les sorties
    char buf_minishell[4096] = {0};
    char buf_bash[4096] = {0};
    
    read(pipe_minishell[0], buf_minishell, sizeof(buf_minishell) - 1);
    read(pipe_bash[0], buf_bash, sizeof(buf_bash) - 1);
    
    close(pipe_minishell[0]);
    close(pipe_bash[0]);

    // Attendre les processus
    int status_minishell, status_bash;
    waitpid(pid_minishell, &status_minishell, 0);
    waitpid(pid_bash, &status_bash, 0);

    // Comparer les statuts et les sorties
    if (WEXITSTATUS(status_minishell) == WEXITSTATUS(status_bash) && 
        strcmp(buf_minishell, buf_bash) == 0) {
        printf("%sTest %s: PASSED%s\n", GREEN, test_name, RESET);
        tests_passed++;
    } else {
        printf("%sTest %s: FAILED%s\n", RED, test_name, RESET);
        printf("Expected output:\n%s\n", buf_bash);
        printf("Got output:\n%s\n", buf_minishell);
        printf("Expected status: %d\n", WEXITSTATUS(status_bash));
        printf("Got status: %d\n", WEXITSTATUS(status_minishell));
    }
}

int main(void)
{
    printf("Running functional tests...\n\n");

    // Tests basiques
    run_test_case("echo test", "Simple echo");
    run_test_case("echo -n test", "Echo with -n option");
    
    // Tests de redirections
    run_test_case("echo hello > test.txt; cat test.txt; rm test.txt", "Output redirection");
    run_test_case("echo first > test.txt; echo second >> test.txt; cat test.txt; rm test.txt", 
                  "Append redirection");
    
    // Tests de pipes
    run_test_case("echo hello | cat", "Simple pipe");
    run_test_case("echo hello | cat | cat | cat", "Multiple pipes");
    
    // Tests des opérateurs && et ||
    run_test_case("true && echo success", "AND operator success");
    run_test_case("false && echo should_not_print", "AND operator failure");
    run_test_case("false || echo success", "OR operator recovery");
    
    // Tests des séquences
    run_test_case("echo first; echo second; echo third", "Command sequence");
    
    // Tests des variables d'environnement
    run_test_case("TEST=value echo $TEST", "Environment variable");
    run_test_case("echo $PATH", "Existing env variable");
    
    // Tests des builtins
    run_test_case("cd /tmp && pwd", "CD builtin");
    run_test_case("cd $HOME && pwd", "CD HOME");
    
    // Tests d'erreurs
    run_test_case("ls nonexistentfile", "Nonexistent file");
    run_test_case("cat < nonexistentfile", "Redirection error");

    printf("\nFunctional tests summary: %d/%d passed\n", tests_passed, test_count);
    return tests_passed == test_count ? 0 : 1;
}
