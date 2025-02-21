#include "all.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "exec/exec.h"

static void process_input(const char *input, struct exec_state *state)
{
    struct lexer *lexer = lexer_init(strdup(input));
    if (!lexer)
        return;

    struct parser *parser = parser_init(lexer);
    if (!parser)
    {
        lexer_free(lexer);
        return;
    }

    struct ast_node *ast = parse_input(parser);
    if (ast)
    {
        exec_ast(ast, state);
        ast_node_free(ast);
    }

    parser_free(parser);
    lexer_free(lexer);
}

static int process_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "minishell: %s: No such file or directory\n", filename);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    struct exec_state *state = exec_init(environ);
    
    while (fgets(buffer, sizeof(buffer), file))
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        process_input(buffer, state);

        if (state->should_exit)
            break;
    }

    int exit_code = state->exit_code;
    exec_free(state);
    fclose(file);
    return exit_code;
}

static int interactive_mode(void)
{
    char buffer[BUFFER_SIZE];
    struct exec_state *state = exec_init(environ);
    
    while (!state->should_exit)
    {
        if (!fgets(buffer, sizeof(buffer), stdin))
            break;

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        process_input(buffer, state);
    }

    int exit_code = state->exit_code;
    exec_free(state);
    return exit_code;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        return process_file(argv[1]);
    else
        return interactive_mode();
}
